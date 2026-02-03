#include "application.h"
#include "GL/gl3w.h"
#include "GL/glcorearb.h"
#include "animationSystem.h"
#include "core/camera.h"
#include "fmt/base.h"
#include "glm/fwd.hpp"
#include "types.h"
#include <glm/gtc/matrix_transform.hpp>
// test for entityMovement
#include "core/road.h"
#include "entityMovement.h"
// #include "utils/graphDebugDump.h"
// #include "utils/graphDebugImage.h"
#include "systems/questsSystem.h"
#include "systems/renderCommon.h"
#include "systems/renderTiles.h"
#include "tradingSystem.h"
#include "utils/worldNodeMapper.h"

#include <random>

#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <set>

#include "core/settlement.h"
#include "events/eventBus.h"
#include "window.h"

#include "ai/behaviorTree.h"
#include "ai/commandRegistry.h"

namespace df {
	static void glfwErrorCallback(int error, const char* description) {
		fmt::println(stderr, "[GLFW Error {}]: {}", error, description);
	}

	::std::optional<Application> Application::init(const CommandLineOptions& options) noexcept {
		if (options.hasHelp())
			return ::std::nullopt;

		Application self;
		fmt::println("\"{}\" version {}.{}", PROJECT_NAME, VERSION_MAJOR, VERSION_MINOR);

		if (options.hasX11())
			glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

		glfwSetErrorCallback(glfwErrorCallback);
		if (!glfwInit()) {
			fmt::println(stderr, "Failed to initialize GLFW");
			return ::std::nullopt;
		}

		auto win = Window::init(1280, 720, PROJECT_NAME);
		if (!win) {
			glfwTerminate();
			return ::std::nullopt;
		}
		self.window = ::std::move(win);

		glfwSetWindowSizeLimits(
			self.window->getHandle(),
			1280, 720,
			GLFW_DONT_CARE, GLFW_DONT_CARE 
		);

		self.window->makeContextCurrent();

		if (gl3wInit()) {
			fmt::println(stderr, "Failed to initialize OpenGL context");
			self.window->deinit();
			glfwTerminate();
			return ::std::nullopt;
		}
		fmt::println("Loaded OpenGL {} & GLSL {}", (char*)glGetString(GL_VERSION), (char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

		self.registry = Registry::init();
		self.eventBus = std::make_shared<EventBus>();
		self.audioEngine = std::make_unique<AudioSystem>(self.eventBus);
		self.aiSystem = std::make_unique<AiSystem>(self.registry);
		self.gameState = std::make_shared<GameState>(self.registry);
		self.gameController = std::make_shared<GameController>(*self.gameState, self.registry);
		self.world = WorldSystem::init(self.window.get(), self.registry, self.audioEngine.get(), *self.gameState);
		// self.physics = PhysicsSystem::init(self.registry, self.audioEngine);
		self.render = RenderSystem::init(self.window.get(), self.registry, self.gameState, self.gameController.get(), self.eventBus.get());
		// Create main menu
		self.mainMenu.init(self.window.get());
		// for testing
		// movement until we have a triggerpoint
		self.movementSystem = std::make_unique<EntityMovementSystem>(self.registry, self.gameState, self.aiSystem);
		// building preview system
		self.buildingPreviewSystem = BuildingPreviewSystem::init(self.window.get(), self.registry, *self.gameState);
		// Create config menu
		self.configMenu.init(self.window.get(), self.registry);

		return self;
	}

	void Application::deinit() noexcept {
		audioEngine.reset();
		movementSystem.reset();
		aiSystem.reset();
		render.deinit();
		delete registry;
		// Poll events one last time to allow GLFW to process any pending cleanup
		// This can help with proper cleanup of Wayland resources
		if (window && window->getHandle()) {
			glfwPollEvents();
		}
		// Explicitly reset the window to ensure it's fully destroyed before glfwTerminate
		// This is important for proper cleanup of Wayland resources
		window.reset();
		glfwTerminate();
	}

	void Application::run() noexcept {
		this->eventBus->applicationRunStarted.emit();

		// Store RenderTextSystem in registry to use it in any other System.
		registry->addSystem<RenderTextSystem>(&render.getRenderTextSystem());
		// Store RenderNofificationSystem in registry to use it in any other System.
		registry->addSystem<RenderNotificationSystem>(&render.getRenderNotificationSystem());
		// Store RenderWeatherSystem in registry to use it in any other System.
		registry->addSystem<RenderWeatherSystem>(&render.getRenderWeatherSystem());

		registry->addSystem<RenderTilesSystem>(&render.getRenderTilesSystem());
		registry->addSystem<EventPresentationSystem>(&render.getEventPresentationSystem());
		registry->addSystem<RenderSettlementMenuSystem>(&render.getRenderSettlementMenuSystem());

		auto* qSys = gameController->getQuestsSystem();
		if (qSys) {
			registry->addSystem<QuestsSystem>(qSys);

			qSys->init(&render.getRenderNotificationSystem());
		}



		if (!this->window || !this->window->getHandle()) {
			std::cerr << "Invalid window or GLFWwindow handle!" << std::endl;
			return;
		}

		window->setResizeCallback([&](GLFWwindow* window, int width, int height) -> void {
			onResizeCallback(window, width, height);
		});

		window->setKeyCallback([&](GLFWwindow* window, int key, int scancode, int action, int mods) -> void {
			onKeyCallback(window, key, scancode, action, mods);
		});

		window->setMouseButtonCallback([&](GLFWwindow* window, int button, int action, int mods) {
			onMouseButtonCallback(window, button, action, mods);
		});

		window->setScrollCallback([&](GLFWwindow* window, double xoffset, double yoffset) {
			onScrollCallback(window, xoffset, yoffset);
		});


		// callbacks so menu can change phase / close window
		mainMenu.setExitCallback([&]() { glfwSetWindowShouldClose(window->getHandle(), true); });
		mainMenu.setStartCallback([&]() { configurateGame(); });

		// callbacks so the config menu can change phase, set world parameters etc.
		// configMenu.setStartCallback([&]() { startGame(); });
		configMenu.setStartCallback(
			[&](int seed,
				int width,
				int height,
				int mode) {
				startGame(seed, width, height, mode);
			});

		// configMenu.setInsularCallback([&]() { setInsular(); });
		// configMenu.setPerlinCallback([&]() { setPerlin(); });

		float delta_time = 0;
		float last_time = static_cast<float>(glfwGetTime());

		glClearColor(0, 0, 0, 1);
		// Force an initial resize to ensure a correct viewport
		int fbWidth, fbHeight;
		glfwGetFramebufferSize(window->getHandle(), &fbWidth, &fbHeight);
		onResizeCallback(window->getHandle(), fbWidth, fbHeight);

		while (!window->shouldClose()) {
			glfwPollEvents();

			float time = static_cast<float>(glfwGetTime());
			delta_time = time - last_time;
			last_time = time;

			types::GamePhase gamePhase = gameState->getPhase();

			// Start turn when first entering PLAY phase -> future TODO: adjust for multiple players + ending game + reentering
			if (gamePhase == types::GamePhase::PLAY && previousGamePhase != types::GamePhase::PLAY) {
				gameController->startTurn();
				fmt::println("Turn started for player {}", gameState->getCurrentPlayerId());
				// Prepare the camera so it can be centered
				world.step(0.0f);
				world.centerCameraOnPoint(movementSystem->getTargetPosition());
			}

			switch (gamePhase) {
			case types::GamePhase::START:
				mainMenu.update(delta_time);
				mainMenu.render();
				break;
			case types::GamePhase::CONFIG:
				configMenu.update(delta_time);
				configMenu.render();
				break;
			case types::GamePhase::PLAY: {
				world.step(delta_time);

				// compute if the user got enough resources and color the corresponding resource green/red if used
				if (world.isSettlementPreviewActive) {
					std::vector<glm::vec3> hudColors = gameState->computeHudResourceColor("settlement");
					render.renderHudSystem.setHudColors(hudColors);
				} else if (world.isRoadPreviewActive) {
					std::vector<glm::vec3> hudColors = gameState->computeHudResourceColor("road");
					render.renderHudSystem.setHudColors(hudColors);
				} else {
					// restore default white
					render.renderHudSystem.setHudColors({{1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}});
				}

				// physics.step(delta_time);
				// physics.handleCollisions(delta_time);
				if (gameState->isGameOver()) {
					window->makeContextCurrent();
					glClearColor(0.24f, 0.299f, 0.475f, 1.0f);
					glClear(GL_COLOR_BUFFER_BIT);
					render.step(delta_time);
					if (!victoryScreenShown) {
						// Render victory notification
						RenderNotificationSystem* notification = registry->getSystem<RenderNotificationSystem>();
						std::string message = fmt::format("\nYou have played for {} rounds!\n\nYou build {} settlements and {} roads.\n",
														  gameState->getRoundNumber(), gameState->getSettlements().size(), gameState->getRoads().size());
						notification->showNotification("You won the Game!", message, {"Back to Menu"});
						fmt::println("Victory! You survived {} rounds.", gameState->getRoundNumber());
						victoryScreenShown = true;
					}
					if (victoryScreenClosed) {
						// reset application once victory screen was closed
						this->reset();
						gameState->resetTutorial();
						gameState->setPhase(types::GamePhase::START);
					}
					break;
				}

				df::AnimationSystem::update(registry, delta_time);

				// update building preview BEFORE rendering
				buildingPreviewSystem.setSettlementPreviewActive(this->world.isSettlementPreviewActive);
				buildingPreviewSystem.setRoadPreviewActive(this->world.isRoadPreviewActive);
				buildingPreviewSystem.step(delta_time);

				window->makeContextCurrent();
				glClearColor(0.24f, 0.299f, 0.475f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);

				render.step(delta_time);
				// ------- only here for testing until we have a triggerpoint for the movement-----------------------------------------------------
				if (movementSystem->getMovementState()) {
					if (!registry->animations.entities.empty()) {
						Entity hero = registry->animations.entities.front();
						movementSystem->moveEntityTo(hero, movementSystem->getTargetPosition(), delta_time);
						if (!movementSystem->getMovementState()) {
							render.renderTilesSystem.selectedTile = -1; // remove tile highlighting after hero arrived
						}
					} else {
						fmt::println("No hero entity available!");
					}
				}
				// ------------------------------------------------------------

				// Only truly end the turn and start a new one when the hero finished walking
				if (awaitingTurnEnd && !movementSystem->getMovementState()) {
					Entity hero = registry->animations.entities.front();
					gameController->endTurn();
					gameController->applyHazard(hero, movementSystem->getTargetPosition());
					gameController->startTurn(); // Start turn for the next player
					awaitingTurnEnd = false;
				}
			} break;
			case types::GamePhase::END:
				break;
			}

			// Update previous phase for next iteration -> future TODO: adjust for multiple players + ending game + reentering
			previousGamePhase = gamePhase;

			window->swapBuffers();
		}
	}

	void Application::toggleMovement() noexcept {
		test = !test;
	}

	void Application::reset() noexcept {
		registry->clear();

		auto* qSys = gameController->getQuestsSystem();
		if (qSys) {
			qSys->reset();
		}

		Entity camEntity = registry->getCamera();

		Camera& cam = registry->cameras.emplace(camEntity);
		cam.isActive = true;
		registry->cameraInputs.emplace(camEntity);

		Entity playerEntity = registry->getPlayer();
		registry->players.emplace(playerEntity);

		registry->positions.emplace(playerEntity, 0.5f, 0.5f);
		registry->velocities.emplace(playerEntity, 0, 0);
		registry->scales.emplace(playerEntity, 1.f, 1.f);
		registry->angles.emplace(playerEntity, 0.f);
		registry->collisionRadius.emplace(playerEntity, 0.1f);

		registry->getScreenDarkness() = 1.f;

		gameState->setRoundNumber(0);
		gameState->setCurrentPlayerId(0);
		gameState->setTurnCount(0);

		registry->animations.emplace(playerEntity);
		registry->tileID.emplace(playerEntity, 0);



		victoryScreenClosed = false;
		victoryScreenShown = false;
		world.reset();
		render.reset();
	}

	void Application::configurateGame() noexcept {
		gameState->setPhase(types::GamePhase::CONFIG);
	}

	void Application::startGame(int seedParam, int widthParam, int heightParam, int mode) noexcept {
		std::string seedName = std::to_string(seedParam);
		std::string widthName = std::to_string(widthParam);
		std::string heightName = std::to_string(heightParam);
		std::string modeName = "";


		// read config from json file
		WorldGeneratorConfig config;
		if (const auto worldGenConfResult = WorldGeneratorConfig::deserialize(); worldGenConfResult.isErr()) {
			std::cerr << worldGenConfResult.unwrapErr() << std::endl;
		} else {
			config = worldGenConfResult.unwrap<>();
		}

		// set config to user input or keep existing config-values if no input was made (== -1)
		if (mode == -1) {
			if (config.generationMode == WorldGeneratorConfig::GenerationMode::INSULAR) {
				modeName = "kept as insular";
			} else if (config.generationMode == WorldGeneratorConfig::GenerationMode::PERLIN) {
				modeName = "kept as perlin";
			}
		} else if (mode == 0) {
			config.generationMode = WorldGeneratorConfig::GenerationMode::INSULAR;
			modeName = "insular";
		} else if (mode == 1) {
			config.generationMode = WorldGeneratorConfig::GenerationMode::PERLIN;
			modeName = "perlin";
		}

		if (seedParam != -1) {
			config.seed = static_cast<unsigned>(seedParam);
		} else {
			seedName = "kept as " + std::to_string(config.seed);
		}

		if (widthParam != -1) {
			config.columns = static_cast<unsigned>(widthParam);
		} else {
			widthName = "kept as " + std::to_string(config.columns);
		}

		if (widthParam != -1) {
			config.rows = static_cast<unsigned>(heightParam);
		} else {
			heightName = "kept as " + std::to_string(config.rows);
		}

		fmt::println("set worldGen parameters to seed: {}, width: {}, height: {}, mode: {}", seedName, widthName, heightName, modeName);


		// write config to json
		const auto path = assets::getAssetPath(assets::JsonFile::WORLD_GENERATION_CONFIGURATION);

		{ // open the stream in an extra block, so the stream gets closed before deserialize tries to open the json
			std::ofstream file(path);
			if (!file) {
				std::cerr << "Could not open config file: " << path << '\n';
				return;
			}
			file << config.serialize().dump(4);
		}
		fmt::println("[DEBUG] config written to file: {}", path.c_str());

		// generate map with the WorldGeneratorConfig
		if (const auto worldGenConfResult = WorldGeneratorConfig::deserialize(); worldGenConfResult.isErr()) {
			fmt::println("[DEBUG] config deserialized");
			std::cerr << worldGenConfResult.unwrapErr() << std::endl;
			fmt::println("[DEBUG] start regenerating...");
			gameState->getMap().regenerate();
		} else {
			gameState->getMap().regenerate(worldGenConfResult.unwrap<>());
		}
		// lets the hero spawn with on a random Tile (water excluded)
		spawnHero();

		const Graph& map = gameState->getMap();
		size_t testId = registry->tileID.get(registry->animations.entities.front());
		auto heroTile = map.getTile(testId);

		auto reachable = map.dijkstra<Tile>(*heroTile);

		fmt::println(
			"[DIJKSTRA TEST] Hero tile {} reaches {} tiles",
			testId,
			reachable.size());

		// Test 1: Pfad zu Tile 18
		auto pathTo18 = gameState->getMap().dijkstraPath(testId, 18);
		fmt::println("[DIJKSTRA PATH TEST] Hero {} -> Tile 18 | Path length: {}", testId, pathTo18.size());
		fmt::print("Path: ");
		for (auto id : pathTo18) {
			fmt::print("{} ", id);
		}
		fmt::println(""); 

		// Test 2: Pfad zu Tile 81
		auto pathTo81 = gameState->getMap().dijkstraPath(testId, 81);
		fmt::println("[DIJKSTRA PATH TEST] Hero {} -> Tile 81 | Path length: {}", testId, pathTo81.size());
		fmt::print("Path: ");
		for (auto id : pathTo81) {
			fmt::print("{} ", id);
		}
		fmt::println(""); 
		

		// 		// This is only for DEBUGGING purposes:
		// #if defined(__unix__) || defined(__linux__)
		// 		fmt::println("Log map config");
		// 		df::utils::writeGraphDebugDump(this->gameState->getMap(), "~/Pictures/debug/graph_debug.txt");
		// 		df::utils::writeGraphDebugImage(this->gameState->getMap(), "~/Pictures/debug/graph_debug.png");
		// #endif

		fmt::println("[DEBUG] regenerated world");
		{
			// only supports one player for now. TODO: if we do multplayer update this.
			Player* player = this->gameState->getPlayer(0);
			if (!player) {
				gameState->addPlayer(Player{});
				player = this->gameState->getPlayer(0);
			}
			player->reset();
			// TODO: add 'test' mode where the player starts with a lot more resources (to show upgrading system)
			player->addResources(types::TileType::FOREST, 10);	 // give player initial wood
			player->addResources(types::TileType::CLAY, 10);	 // give player initial clay
			player->addResources(types::TileType::MOUNTAIN, 10); // give player initial stone
			player->addResources(types::TileType::FIELD, 10);	 // give player initial grain
			player->addResources(types::TileType::GRASS, 10);	 // give player initial grass (cattle)

			fmt::println("[DEBUG] resources distributed to player");

			tradingSystem.init(&render.getRenderNotificationSystem(), player);

			world.setTradeCallback([this]() {
				tradingSystem.startTrading();
			});

			const int width = gameState->getMap().getMapWidth();
			const int height = gameState->getMap().getTileCount() / width;

			// auto randomEngine = std::default_random_engine(std::random_device()());
			// auto uniformDistribution = std::uniform_int_distribution();


			// Remove this if we dont want to be the water tiles already explored
			// same for ice
			for (int row = 0; row < height; ++row) {
				for (int col = 0; col < width; ++col) {
					size_t tileId = row * width + col;
					const TileHandle tile = gameState->getMap().getTile(tileId);

					if (tile && tile->getType() == types::TileType::WATER) {
						gameState->getPlayer(0)->exploreTile(tileId);
					} else if (tile && tile->getType() == types::TileType::ICE) {
						gameState->getPlayer(0)->exploreTile(tileId);
					}
				}
			}

			// Discover a radius of one tile around the hero
			if (!registry->animations.entities.empty()) {
				Entity hero = registry->animations.entities.front();
				if (registry->tileID.has(hero)) {
					int heroTileId = static_cast<int>(registry->tileID.get(hero));

					int centerRow = heroTileId / width;
					int centerCol = heroTileId % width;
					int radius = 1;
					if (centerRow % 2 == 0) {
						for (int r = -radius; r <= radius; ++r) {
							for (int c = -radius; c <= radius; ++c) {
								int targetRow = centerRow + r;
								int targetCol = centerCol + c;

								if (!((c == 1 && r == 1) || (c == 1 && r == -1))) {
									if (targetRow >= 0 && targetRow < height && targetCol >= 0 && targetCol < width) {
										size_t idToExplore = static_cast<size_t>(targetRow * width + targetCol);
										player->exploreTile(idToExplore);
									}
								}
							}
						}
					} else {
						for (int r = -radius; r <= radius; ++r) {
							for (int c = -radius; c <= radius; ++c) {
								int targetRow = centerRow + r;
								int targetCol = centerCol + c;
								if (!((c == -1 && r == -1) || (c == -1 && r == 1))) {
									if (targetRow >= 0 && targetRow < height && targetCol >= 0 && targetCol < width) {
										size_t idToExplore = static_cast<size_t>(targetRow * width + targetCol);
										player->exploreTile(idToExplore);
									}
								}
							}
						}
					}
				}
			}
		}
		if (const auto result = render.renderTilesSystem.updateMap(); result.isErr()) {
			std::cerr << result.unwrapErr() << std::endl;
		}
		render.renderHeroSystem.updateDimensionsFromMap();

		gameState->initTutorial(); // Init the Tutorial
		gameState->setPhase(types::GamePhase::PLAY);
		fmt::println("[DEBUG] Application::startGame completed");
	}

	void Application::onKeyCallback(GLFWwindow* windowParam, int key, int scancode, int action, int mods) noexcept {
		// For testing purposes
		aiSystem->onKeyCallback(windowParam, key, scancode, action, mods);
		int currentQuestId;
		types::GamePhase gamePhase = gameState->getPhase();
		switch (gamePhase) {
		case types::GamePhase::START:
			mainMenu.onKeyCallback(windowParam, key, scancode, action, mods);
			break;
		case types::GamePhase::CONFIG:
			configMenu.onKeyCallback(windowParam, key, scancode, action, mods);
			break;
		case types::GamePhase::PLAY:
			if (render.eventPresentationSystem.currentEvent) {
				return;
			}

			if(action == GLFW_PRESS && key == GLFW_KEY_ENTER){
				if (!gameState->isGameOver() && !movementSystem->getMovementState() && !render.renderNotificationSystem.isActive()) {
            
					auto* step = this->gameState->getCurrentTutorialStep();
					if (step && step->id == TutorialStepId::MOVE_HERO) {
						this->gameState->completeCurrentTutorialStep();
					}

					Entity hero = registry->animations.entities.front();
					if (!registry->hazards.has(hero)) {
						movementSystem->toggleMovementState();
					}
					awaitingTurnEnd = true; 
				}
			}

			// finish quest once requirements met
			currentQuestId = gameController->getQuestsSystem()->getCurrentShowingQuestId();
			gameController->claimQuestReward(currentQuestId);

			if (render.renderSettlementMenuSystem.isActive()) {
				render.renderSettlementMenuSystem.close();
				selectedSettlementId = SIZE_MAX;
			}
			if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
				if (render.renderNotificationSystem.isActive()) {
					gameController->getQuestsSystem()->setCurrentQuest();
					if(world.getShowTrade()){
						world.setShowTrade(false);
					}
					render.renderNotificationSystem.close();
					selectedSettlementId = SIZE_MAX;
					world.escPressed();
					return;
				}
			}
			world.onKeyCallback(windowParam, key, scancode, action, mods);
			render.onKeyCallback(windowParam, key, scancode, action, mods);
			break;
		case types::GamePhase::END:
			break;
		}
	}

	void Application::spawnHero() noexcept {
		Entity hero;
		if (!registry->animations.entities.empty()) {
			hero = registry->animations.entities.front();
		} else {
			hero = registry->getPlayer();
			registry->animations.emplace(hero);
		}

		Graph& map = gameState->getMap();
		int mapWidth = map.getMapWidth();
		int mapHeight = map.getTileCount() / mapWidth;

		std::random_device rd;
		std::mt19937 rng(rd());
		std::uniform_int_distribution<int> dist(0, mapWidth * mapHeight - 1);
		int randomTileID;
		do {
			randomTileID = dist(rng);
		} while (map.getTile(randomTileID)->getType() == types::TileType::WATER);

		glm::vec2 startPosition = movementSystem->getTileWorldPosition(randomTileID);
		fmt::println("Hero spawned at TileID: {} with coords: X: {}, Y: {}", randomTileID, startPosition.x, startPosition.y);

		if (registry->positions.has(hero)) {
			registry->positions.get(hero) = startPosition;
		} else {
			registry->positions.emplace(hero, startPosition);
		}

		if (registry->tileID.has(hero)) {
			registry->tileID.get(hero) = randomTileID;
		} else {
			registry->tileID.emplace(hero, randomTileID);
		}
		movementSystem->setTarget(randomTileID, hero);
	}

	void Application::onMouseButtonCallback(GLFWwindow* windowParam, int button, int action, int mods) noexcept {
		types::GamePhase gamePhase = gameState->getPhase();

		switch (gamePhase) {
		case types::GamePhase::START:
			mainMenu.onMouseButtonCallback(windowParam, button, action, mods);
			break;
		case types::GamePhase::CONFIG:
			configMenu.onMouseButtonCallback(windowParam, button, action, mods);
			break;
		case types::GamePhase::PLAY: {
			double xpos, ypos;
			glfwGetCursorPos(windowParam, &xpos, &ypos);

			int winWidth, winHeight;
			glfwGetWindowSize(windowParam, &winWidth, &winHeight);

			int fbWidth, fbHeight;
			glfwGetFramebufferSize(windowParam, &fbWidth, &fbHeight);


			float xScale = (winWidth > 0) ? (float)fbWidth / winWidth : 1.f;
			float yScale = (winHeight > 0) ? (float)fbHeight / winHeight : 1.f;

			float mouseX = static_cast<float>(xpos * xScale);
			float mouseY = static_cast<float>(ypos * yScale);

			glm::vec2 mouse{
				mouseX,
				static_cast<float>(window->getWindowExtent().y) - mouseY};


			// Check if any Notification buttons were pressed
			std::string pressedButton = render.renderNotificationSystem.onMouseButton(mouse, button, action);
			// If any button was pressed continue
			if (!pressedButton.empty()) {
				std::cout << "Button: " << pressedButton << " was pressed" << std::endl;

				// finish quest once requirements met
				int currentId = gameController->getQuestsSystem()->getCurrentShowingQuestId();
				gameController->claimQuestReward(currentId);

				// TODO: add actions for button pressed in notifications
				if (pressedButton == "Wood" || pressedButton == "Stone" ||
					pressedButton == "Clay" || pressedButton == "Wool" || pressedButton == "Grain") {
					tradingSystem.handleOptionClicked(pressedButton);
					if(world.getShowTrade()){
						world.setShowTrade(false);
					} 
				}
				if (pressedButton == "Pay ressources") {
					gameController->payForHazard();
					render.eventPresentationSystem.endEvent();
				}
				if (pressedButton == "Wait") {
					render.eventPresentationSystem.endEvent();
				}
				// Quests
				if (pressedButton == "Next Quest") {
					this->onKeyCallback(windowParam, GLFW_KEY_Q, 0, GLFW_PRESS, 0);
				}

				if(pressedButton == "Close"){
					gameController->getQuestsSystem()->setCurrentQuest();
					world.escPressed();
				}

				if(pressedButton == "Cancel"){
					world.setShowTrade(false);
				}

				if (pressedButton == "Back to Menu") {
					victoryScreenClosed = true; // close victory screen and go back to menu
				}
				return; // notification clicked -> no further actions (including movement) for now
			}
			if (render.renderNotificationSystem.isActive() || render.eventPresentationSystem.currentEvent) {
				return;
			}
			if (render.renderSettlementMenuSystem.isActive()) {
				if (render.renderSettlementMenuSystem.onMouseButton(mouse, button, action)) {
					return;
				}
			}

			size_t hoveredSettlementId = SIZE_MAX;
			if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS &&
				!this->world.isSettlementPreviewActive && !this->world.isRoadPreviewActive) {
				Camera& cam = registry->cameras.get(registry->getCamera());
				Viewport viewport{glm::uvec2(0), window->getWindowExtent()};
				glm::vec2 cursorScreenPos = window->getCursorPosition();
				glm::vec2 cursorWorldOffset = screenToWorldCoordinates(
					cursorScreenPos,
					viewport,
					glm::vec2(cam.viewWidth, cam.viewHeight));
				glm::vec2 cursorWorldPos = cam.position + cursorWorldOffset;

				float closestDistance = (std::numeric_limits<float>::max)();
				size_t currentPlayerId = gameState->getCurrentPlayerId();
				for (Entity e : registry->settlements.entities) {
					if (!registry->positions.has(e) || !registry->settlements.has(e) || !registry->scales.has(e)) {
						continue;
					}
					const Settlement& settlement = registry->settlements.get(e);
					if (settlement.getPlayerId() != currentPlayerId) {
						continue;
					}

					const glm::vec2& worldPos = registry->positions.get(e);
					const glm::vec2& scale = registry->scales.get(e);
					float hitRadius = std::max(scale.x, scale.y) * 0.6f;
					float distance = glm::distance(cursorWorldPos, worldPos);
					if (distance < closestDistance && distance <= hitRadius) {
						closestDistance = distance;
						hoveredSettlementId = settlement.getId();
					}
				}
			}

			if (render.renderSettlementMenuSystem.isActive() &&
				button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS &&
				hoveredSettlementId == SIZE_MAX &&
				!render.renderSettlementMenuSystem.isPointInsideMenu(mouse)) {
				render.renderSettlementMenuSystem.close();
				selectedSettlementId = SIZE_MAX;
				return;
			}

			// START Lock all following interactions with the game while the hero is still moving
			if (!movementSystem->getMovementState()) {
				// Check if End Turn button was clicked -> needs to be adjusted for AI-players
				if (render.renderHudSystem.wasEndTurnClicked(mouse, button, action)) {
					if (!gameState->isGameOver()) {

						auto* step = this->gameState->getCurrentTutorialStep();
						if (step && step->id == TutorialStepId::MOVE_HERO) {
							this->gameState->completeCurrentTutorialStep();
						}

						Entity hero = registry->animations.entities.front();
						// TODO: For multiplayer check only for active player for hazards
						if (!registry->hazards.has(hero) && world.getMouseX() >= 0 && world.getMouseY() >= 0) {
							movementSystem->toggleMovementState();
							fmt::println("Hero destination: {},{}", movementSystem->getTargetPosition().x, movementSystem->getTargetPosition().y);
						}
						awaitingTurnEnd = true;
						return;
					}
				}

				if (render.renderHudSystem.onMouseButton(mouse, button, action)) {
					// Check if any buttons on the side hud were pressed
					if (!render.renderHudSystem.getLastSideHudButtonPressed().empty()) {
						std::string SideHudButton = render.renderHudSystem.getLastSideHudButtonPressed();

						if (SideHudButton == "Trade") {
							onKeyCallback(windowParam, GLFW_KEY_T, 0, GLFW_PRESS, 0);
						} else if (SideHudButton == "Quest") {
							onKeyCallback(windowParam, GLFW_KEY_Q, 0, GLFW_PRESS, 0);
						} else if (SideHudButton == "Cost") {
							onKeyCallback(windowParam, GLFW_KEY_C, 0, GLFW_PRESS, 0);
						} else if (SideHudButton == "Keybindings") {
							onKeyCallback(windowParam, GLFW_KEY_K, 0, GLFW_PRESS, 0);
						}
					}
					return;
				}

				// Settlement management menu
				if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS &&
					!this->world.isSettlementPreviewActive && !this->world.isRoadPreviewActive) {
					if (hoveredSettlementId != SIZE_MAX) {
						selectedSettlementId = hoveredSettlementId;
						render.renderSettlementMenuSystem.showMenu(hoveredSettlementId);
						return;
					}
				}

				// TODO: refactor...
				// Handle building placement -> ONLY possible when preview is active
				if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
					if (this->world.isSettlementPreviewActive || this->world.isRoadPreviewActive) {
						fmt::println("Building placement started...");

						Entity previewEntity = buildingPreviewSystem.getPreviewEntity();
						if (!registry->positions.has(previewEntity)) {
							fmt::println("No preview entity found");
							return;
						}
						const glm::vec2& cameraRelativePos = registry->positions.get(previewEntity);

						// Reconstruct absolute world position from camera-relative position
						Camera& cam = registry->cameras.get(registry->getCamera());
						glm::vec2 worldPos = cam.position + cameraRelativePos;

						const Graph& map = this->gameState->getMap();

						size_t currentPlayerId = this->gameState->getCurrentPlayerId();
						fmt::println("Current player ID: {}", currentPlayerId);

						if (this->world.isSettlementPreviewActive) {
							fmt::println("Checking if player can build settlement at world position {},{}", worldPos.x, worldPos.y);
							// Find closest vertex for settlement placement
							auto vertexIdOpt = WorldNodeMapper::findClosestVertexToWorldPos(worldPos, map);
							if (vertexIdOpt.has_value()) {
								fmt::println("Closest vertex found at {}", vertexIdOpt.value());
								size_t vertexId = vertexIdOpt.value();
								if (this->gameController->canBuildSettlement(currentPlayerId, vertexId)) { // validate player can build settlement
									fmt::println("Player can build settlement at vertex {}", vertexId);
									const auto settlementCost = this->gameState->getCurrentSettlementCost();
									bool success = this->gameController->buildSettlement(currentPlayerId, vertexId, settlementCost);

									if (success) {
										fmt::println("Settlement built at vertex {}", vertexId);
										this->world.isSettlementPreviewActive = false;
									} else {
										fmt::println("Failed to build settlement at vertex {}", vertexId);
									}

								} else {
									fmt::println("Cannot build settlement at vertex {}: insufficient resources or invalid placement", vertexId);
								}
							} else
								fmt::println("No closest vertex found");

						} else if (this->world.isRoadPreviewActive) {
							fmt::println("Checking if player can build road at world position {},{}", worldPos.x, worldPos.y);
							// Find closest edge for road placement
							auto edgeIdOpt = WorldNodeMapper::findClosestEdgeToWorldPos(worldPos, map);
							if (edgeIdOpt.has_value()) {
								fmt::println("Closest edge found at {}", edgeIdOpt.value());
								size_t edgeId = edgeIdOpt.value();

								if (gameController->canBuildRoad(currentPlayerId, edgeId)) { // validate player can build road
									fmt::println("Player can build road at edge {}", edgeId);
									const auto roadCost = this->gameState->getCurrentRoadCost();
									bool success = gameController->buildRoad(currentPlayerId, edgeId, RoadLevel::Path, roadCost);
									if (success) {
										fmt::println("Road built at edge {}", edgeId);
										this->world.isRoadPreviewActive = false;
									} else {
										fmt::println("Failed to build road at edge {}", edgeId);
									}

								} else {
									fmt::println("Cannot build road at edge {}: insufficient resources or invalid placement", edgeId);
								}

							} else
								fmt::println("No closest edge found");
						}

						return; // ignore other mouse callbacks when placing buildings...
					}
				}

				if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
					glm::vec2 mouseCoords = glm::vec2(mouseX, mouseY);
					auto extent = this->window->getWindowExtent();

					auto tileId = render.renderTilesSystem.getTileIdAtPosition(mouseCoords.x, extent.y - mouseCoords.y);
					auto mapId = render.renderTilesSystem.tileIdToMapId(tileId);
					fmt::println("Picked: TileId {} / MapId {} at mouse ({}, {})", tileId, mapId, mouseCoords.x, mouseCoords.y);

					if (mapId >= 0 && !movementSystem->isEntityMoving()) {
						//  TODO: For multiplayer use hero of active player
						Entity hero = registry->animations.entities.front();
						movementSystem->setTarget(mapId, hero);
					}
				}

				world.onMouseButtonCallback(windowParam, button, action, mods);
				render.onMouseButtonCallback(windowParam, button, action, mods);
			} // END Lock for movement
		} break;
		case types::GamePhase::END:
			break;
		}
	}

	void Application::onScrollCallback(GLFWwindow* windowParam, double xoffset, double yoffset) noexcept {
		types::GamePhase gamePhase = gameState->getPhase();

		switch (gamePhase) {
		case types::GamePhase::START:
			break;
		case types::GamePhase::CONFIG:
			break;
		case types::GamePhase::PLAY:
			world.onScrollCallback(windowParam, xoffset, yoffset);
			break;
		case types::GamePhase::END:
			break;
		}
	}

	void Application::onResizeCallback(GLFWwindow* windowParam, int width, int height) noexcept {
		if (width <= 0 || height <= 0) // prevent crashing window under windows when minimizing
			return;
		mainMenu.onResizeCallback(windowParam, width, height);
		render.onResizeCallback(windowParam, width, height);
		render.renderHudSystem.onResizeCallback(windowParam, width, height);
		configMenu.onResizeCallback(windowParam, width, height);
		render.renderNotificationSystem.onResizeCallback(windowParam, width, height);
		render.eventPresentationSystem.onResizeCallback(windowParam, width, height);
	}
} // namespace df
