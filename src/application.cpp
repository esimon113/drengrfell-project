#include "application.h"
#include "GL/gl3w.h"
#include "GL/glcorearb.h"
#include "fmt/base.h"
#include "glm/fwd.hpp"
#include "animationSystem.h"
#include "types.h"
#include "core/camera.h"
#include <glm/gtc/matrix_transform.hpp>
// test for entityMovement
#include "entityMovement.h"
#include "utils/worldNodeMapper.h"
#include "core/road.h"


#include <iostream>
#include <fstream>




namespace df {
	static void glfwErrorCallback(int error, const char* description) {
		fmt::println(stderr, "[GLFW Error {}]: {}", error, description);
	}

	::std::optional<Application> Application::init(const CommandLineOptions& options) noexcept {
		if (options.hasHelp()) return ::std::nullopt;

		Application self;
		fmt::println("\"{}\" version {}.{}", PROJECT_NAME, VERSION_MAJOR, VERSION_MINOR);

		if (options.hasX11()) glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

		glfwSetErrorCallback(glfwErrorCallback);
		if (!glfwInit()) {
			fmt::println(stderr, "Failed to initialize GLFW");
			return ::std::nullopt;
		}

		::std::optional<Window*> win = Window::init(600, 600, PROJECT_NAME);
		if (!win) {
			glfwTerminate();
			return ::std::nullopt;
		}
		self.window = ::std::move(*win);

		self.window->makeContextCurrent();

		if (gl3wInit()) {
			fmt::println(stderr, "Failed to initialize OpenGL context");
			self.window->deinit();
			glfwTerminate();
			return ::std::nullopt;
		}
		fmt::println("Loaded OpenGL {} & GLSL {}", (char*)glGetString(GL_VERSION), (char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

		self.audioEngine = new AudioSystem;
		*self.audioEngine = AudioSystem::init();
		self.registry = Registry::init();
		self.gameState = std::make_shared<GameState>(self.registry);
		self.gameController = std::make_shared<GameController>(*self.gameState);
		self.world = WorldSystem::init(self.window, self.registry, self.audioEngine, *self.gameState);
		// self.physics = PhysicsSystem::init(self.registry, self.audioEngine);
		self.render = RenderSystem::init(self.window, self.registry, self.gameState);
		// Create main menu
		self.mainMenu.init(self.window);
		// for testing hero movement until we have a triggerpoint
		self.movementSystem = EntityMovementSystem::init(self.registry, *self.gameState);
		// building preview system
		self.buildingPreviewSystem = BuildingPreviewSystem::init(self.window, self.registry, *self.gameState);
		// Create config menu
		self.configMenu.init(self.window, self.registry);

		return self;
	}

	void Application::deinit() noexcept {
		render.deinit();
		delete registry;
		window->deinit();
		delete window;
		glfwTerminate();
	}

	void Application::run() noexcept {
		ma_sound* music = this->audioEngine->getBackgroundMusic();
		ma_sound_set_looping(music, MA_TRUE);
		ma_sound_start(music);

		// Store RenderTextSystem in registry to use it in any other System.
		registry->addSystem<RenderTextSystem>(&render.getRenderTextSystem());
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
		//configMenu.setStartCallback([&]() { startGame(); });
		configMenu.setStartCallback(
			[&](int seed,
				int width,
				int height,
				int mode)
			{
				startGame(seed, width, height, mode);
			}
		);

		//configMenu.setInsularCallback([&]() { setInsular(); });
		//configMenu.setPerlinCallback([&]() { setPerlin(); });

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
				case types::GamePhase::PLAY:
				{
					world.step(delta_time);
					// physics.step(delta_time);
					// physics.handleCollisions(delta_time);

					if(gameState->isGameOver()){
						fmt::println("Victory! You survived {} rounds.", gameState->getRoundNumber());
						this->reset();
						gameState->setPhase(types::GamePhase::START);
						break;
					}

					df::AnimationSystem::update(registry, delta_time);

					// update building preview BEFORE rendering
					buildingPreviewSystem.setSettlementPreviewActive(this->world.isSettlementPreviewActive);
					buildingPreviewSystem.setRoadPreviewActive(this->world.isRoadPreviewActive);
					buildingPreviewSystem.step(delta_time);

					window->makeContextCurrent();
					glClearColor(0.5f,0.5f,0.5f,1.0f);
					glClear(GL_COLOR_BUFFER_BIT);

					render.step(delta_time);
					// ------- only here for testing until we have a triggerpoint for the movement-----------------------------------------------------
					if (movementSystem.getMovementState()) {
						if (!registry->animations.entities.empty()) {

							glm::vec2 mouseCoords = glm::vec2(world.getMouseX(), world.getMouseY());
							auto extent = this->window->getWindowExtent();

							auto tileId = render.renderTilesSystem.getTileIdAtPosition(mouseCoords.x, extent.y - mouseCoords.y);
							auto mapId = render.renderTilesSystem.tileIdToMapId(tileId);
							//fmt::println("Picked: TileId {} / MapId {} at mouse ({}, {})", tileId, mapId, mouseCoords.x, mouseCoords.y);

							glm::vec2 tilePosition = movementSystem.getTileWorldPosition(mapId);
							//fmt::println("Tile Position: ({},{})", tilePosition.x, tilePosition.y);

							Entity hero = registry->animations.entities.front();
							glm::vec2 targetPos = tilePosition;
							//glm::vec2 currentTargetPos = targetPos;

							movementSystem.moveEntityTo(hero, targetPos, delta_time);
						}
						else {
							fmt::println("No hero entity available!");
						}
					}
					// ------------------------------------------------------------
				}
					break;
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

		registry->animations.emplace(playerEntity);


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
		}
		else {
			config = worldGenConfResult.unwrap<>();
		}

		// set config to user input or keep existing config-values if no input was made (== -1)
		if (mode == -1) {
			if (config.generationMode == WorldGeneratorConfig::GenerationMode::INSULAR) {
				modeName = "kept as insular";
			}
			else if (config.generationMode == WorldGeneratorConfig::GenerationMode::PERLIN) {
				modeName = "kept as perlin";
			}
		} else if (mode == 0) {
			config.generationMode = WorldGeneratorConfig::GenerationMode::INSULAR;
			modeName = "insular";
		}
		else if (mode == 1) {
			config.generationMode = WorldGeneratorConfig::GenerationMode::PERLIN;
			modeName = "perlin";
		}

		if (seedParam != -1) {config.seed = static_cast<unsigned>(seedParam);}
		else {seedName = "kept as " + std::to_string(config.seed);}

		if (widthParam != -1) {config.columns = static_cast<unsigned>(widthParam);}
		else {widthName = "kept as " + std::to_string(config.columns);}

		if (widthParam != -1) {config.rows = static_cast<unsigned>(heightParam);}
		else {heightName = "kept as " + std::to_string(config.rows);}

		fmt::println("set worldGen parameters to seed: {}, width: {}, height: {}, mode: {}", seedName, widthName, heightName, modeName);


		// write config to json
		const auto path = assets::getAssetPath(assets::JsonFile::WORLD_GENERATION_CONFIGURATION);

		{	// open the stream in an extra block, so the stream gets closed before deserialize tries to open the json
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
		}
		else {
			gameState->getMap().regenerate(worldGenConfResult.unwrap<>());
		}
		fmt::println("[DEBUG] regenerated world");
		{
			// only supports one player for now. TODO: if we do multplayer update this.
			if (gameState->getPlayer(0)) {
				Player* player = gameState->getPlayer(0);
				player->reset();
				player->addResources(types::TileType::FOREST, 100);				// give player 100 wood
				player->addResources(types::TileType::MOUNTAIN, 100);			// give player 100 stone
				player->addResources(types::TileType::FIELD, 50);				// give player 50 grain
			}
			else {
				Player player{};
				player.addResources(types::TileType::FOREST, 100);				// give player 100 wood
				player.addResources(types::TileType::MOUNTAIN, 100);			// give player 100 stone
				player.addResources(types::TileType::FIELD, 50);				// give player 50 grain
				gameState->addPlayer(player);
			}
			fmt::println("[DEBUG] resources distributed to player");
			const int width = gameState->getMap().getMapWidth();
			const int height = gameState->getMap().getTileCount() / width;

			auto randomEngine = std::default_random_engine(std::random_device()());
			auto uniformDistribution = std::uniform_int_distribution();

			for (int row = 0; row < height; ++row) {
				for (int col = 0; col < width; ++col) {
					if (uniformDistribution(randomEngine) % 4 != 0) {
						gameState->getPlayer(0)->exploreTile(row * width + col);
					}
				}
			}
			fmt::println("[DEBUG] randomly explored tiles for player");
		}
		if (const auto result = render.renderTilesSystem.updateMap(); result.isErr()) {
			std::cerr << result.unwrapErr() << std::endl;
		}
		render.renderHeroSystem.updateDimensionsFromMap();

		gameState->initTutorial();	// Init the Tutorial
		gameState->setPhase(types::GamePhase::PLAY);
		fmt::println("[DEBUG] Application::startGame completed");
	}

	void Application::onKeyCallback(GLFWwindow* windowParam, int key, int scancode, int action, int mods) noexcept {
		types::GamePhase gamePhase = gameState->getPhase();
		auto* step = this->gameState->getCurrentTutorialStep();
		switch (gamePhase) {
		case types::GamePhase::START:
			mainMenu.onKeyCallback(windowParam, key, scancode, action, mods);
			break;
		case types::GamePhase::CONFIG:
			configMenu.onKeyCallback(windowParam, key, scancode, action, mods);
			break;
		case types::GamePhase::PLAY:
			world.onKeyCallback(windowParam, key, scancode, action, mods);
			render.onKeyCallback(windowParam, key, scancode, action, mods);
			// Update Tutorial if step == moveCamera
			if (step && step->id == TutorialStepId::MOVE_CAMERA) {
				if (action == GLFW_PRESS && (key == GLFW_KEY_W || key == GLFW_KEY_S || key == GLFW_KEY_A || key == GLFW_KEY_D)) {
					this->gameState->completeCurrentTutorialStep();
				}
			}
			break;
		case types::GamePhase::END:
			break;
		}
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
				static_cast<float>(window->getWindowExtent().y) - mouseY
			};

			// Check if End Turn button was clicked -> needs to be adjusted for AI-players
			if (!movementSystem.getMovementState()) {
				if (render.renderHudSystem.wasEndTurnClicked(mouse, button, action)) {
					gameController->endTurn();
					movementSystem.toggleMovementState();
					gameController->startTurn(); // Start turn for the next player
					return;
				}
			}

			if (render.renderHudSystem.onMouseButton(mouse, button, action))
				return;

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

					// TODO: This is just temporary...
					// Settlement: 1 WOOD, 1 CLAY, 1 GRASS
					const std::vector<int> settlementCost = {
						0,  // EMPTY
						0,  // WATER
						1,  // FOREST (wood)
						1,  // GRASS
						0,  // MOUNTAIN
						0,  // FIELD
						1,  // CLAY
						0   // ICE
					};
					// Road: 1 WOOD
					const std::vector<int> roadCost = {
						0,  // EMPTY
						0,  // WATER
						1,  // FOREST (wood)
						0,  // GRASS
						0,  // MOUNTAIN
						0,  // FIELD
						0,  // CLAY
						0   // ICE
					};

					if (this->world.isSettlementPreviewActive) {
						fmt::println("Checking if player can build settlement at world position {},{}", worldPos.x, worldPos.y);
						// Find closest vertex for settlement placement
						auto vertexIdOpt = WorldNodeMapper::findClosestVertexToWorldPos(worldPos, map);
						if (vertexIdOpt.has_value()) {
							fmt::println("Closest vertex found at {}", vertexIdOpt.value());
							size_t vertexId = vertexIdOpt.value();
							if (gameController->canBuildSettlement(currentPlayerId, vertexId)) { // validate player can build settlement
								fmt::println("Player can build settlement at vertex {}", vertexId);
								bool success = gameController->buildSettlement(currentPlayerId, vertexId, settlementCost);

								if (success) {
									fmt::println("Settlement built at vertex {}", vertexId);
									this->world.isSettlementPreviewActive = false;
								} else {
									fmt::println("Failed to build settlement at vertex {}", vertexId);
								}

							} else {
								fmt::println("Cannot build settlement at vertex {}: insufficient resources or invalid placement", vertexId);
							}
						} else fmt::println("No closest vertex found");

					} else if (this->world.isRoadPreviewActive) {
						fmt::println("Checking if player can build road at world position {},{}", worldPos.x, worldPos.y);
						// Find closest edge for road placement
						auto edgeIdOpt = WorldNodeMapper::findClosestEdgeToWorldPos(worldPos, map);
						if (edgeIdOpt.has_value()) {
							fmt::println("Closest edge found at {}", edgeIdOpt.value());
							size_t edgeId = edgeIdOpt.value();

							if (gameController->canBuildRoad(currentPlayerId, edgeId)) { // validate player can build road
								fmt::println("Player can build road at edge {}", edgeId);
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

						} else fmt::println("No closest edge found");
					}

					return; // ignore other mouse callbacks when placing buildings...
				}
			}

			world.onMouseButtonCallback(windowParam, button, action, mods);
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
		mainMenu.onResizeCallback(windowParam, width, height);
		render.onResizeCallback(windowParam, width, height);
		render.renderHudSystem.onResizeCallback(windowParam, width, height);
		configMenu.onResizeCallback(windowParam, width, height);

	}
}
