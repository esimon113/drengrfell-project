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
#include "core/hazards.h"
#include "core/hero.h"
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
		self.aiSystem = std::make_unique<AiSystem>(self.registry, self.eventBus);
		self.gameState = std::make_shared<GameState>(self.registry);
		self.gameController = std::make_shared<GameController>(*self.gameState);
		self.world = WorldSystem::init(self.window.get(), self.registry, self.audioEngine.get(), *self.gameState);
		// self.physics = PhysicsSystem::init(self.registry, self.audioEngine);
		self.render = RenderSystem::init(self.window.get(), self.registry, self.gameState, self.gameController.get(), self.eventBus.get());
		// Create main menu
		self.mainMenu.init(self.window.get());
		self.netMutex = std::make_unique<std::mutex>();
		self.serverHost = options.getHost();
		self.serverPort = options.getPort();
		self.playerName = options.getPlayerName();
		self.soloRequested = options.isSolo();
		// for testing
		// movement until we have a triggerpoint
		self.movementSystem = std::make_unique<EntityMovementSystem>(self.registry, self.gameState, self.aiSystem, self.eventBus);
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
		registry->addSystem<AiSystem>(aiSystem.get());

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
			drainNet();

			float time = static_cast<float>(glfwGetTime());
			delta_time = time - last_time;
			last_time = time;

			types::GamePhase gamePhase = gameState->getPhase();

			// Start turn when first entering PLAY phase -> future TODO: adjust for multiple players + ending game + reentering
			if (gamePhase == types::GamePhase::PLAY && previousGamePhase != types::GamePhase::PLAY) {
				if (!midgard || !midgard->isConnected()) {
					gameController->startTurn();
				}
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
						size_t winnerId = gameState->getCurrentPlayerId();
						std::string leaderboard;

						for (size_t i = 0; i < gameState->getPlayerCount(); ++i) {
							auto p = gameController->getPlayerbyId(i);
							if (p) {
								leaderboard += fmt::format("Finished in {} rounds with {} points.\n",
															gameState->getRoundNumber(),
															p->getHeroPoints());
							}
						}

						RenderNotificationSystem* notification = registry->getSystem<RenderNotificationSystem>();

						notification->showNotification("FINISHED!", leaderboard, {"Back to Menu"});

						fmt::println("Game ended. Winner: Player {}", winnerId);
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
							render.renderTilesSystem.setSelectedTile(-1);
						}
					} else {
						fmt::println("No hero entity available!");
					}
				}
				// ------------------------------------------------------------

				if (hazardPresentationPending && !movementSystem->getMovementState()) {
					presentHazardState(hazardPlayerId, hazardHadBefore, hazardPreviousName);
					hazardPresentationPending = false;
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
		if (joining) {
			return;
		}

		pendingSeed = seedParam;
		pendingWidth = widthParam;
		pendingHeight = heightParam;
		pendingMode = mode;
		configSent = false;
		readySent = false;
		startSent = false;
		sessionMapReady = false;
		heroPlaced = false;
		tradingReady = false;
		hazardPresentationPending = false;

		if (!midgard) {
			midgard = std::make_unique<df::bifrost::Midgard>();
			render.renderSettlementMenuSystem.setMidgard(midgard.get());
			gameState->setTutorialReporter([this](TutorialStepId id) {
				if (midgard && midgard->isConnected()) {
					midgard->reportTutorialEvent(static_cast<int>(id));
				}
			});
		}

		midgard->setLobbyStateCallback([this](const df::bifrost::LobbyState& lobby) {
			enqueueNet([this, lobby] { onLobby(lobby); });
		});
		midgard->setGameStateCallback([this](const nlohmann::json& state) {
			enqueueNet([this, state] { onAuthoritativeState(state); });
		});
		midgard->setActionResultCallback([this](uint32_t, bool success, const std::optional<df::bifrost::ErrorInfo>& error) {
			enqueueNet([this, success, error] { onActionResult(success, error); });
		});
		midgard->setConnectionCallback([this](bool connected, const std::string& reason) {
			enqueueNet([this, connected, reason] {
				if (!connected) {
					fmt::println(stderr, "Disconnected from server: {}", reason);
					joining = false;
				}
			});
		});

		if (!midgard->isConnected() && !midgard->connect(serverHost, serverPort)) {
			fmt::println(stderr, "Could not connect to {}:{}", serverHost, serverPort);
			fmt::println(stderr, "Start drengrfell_server first, then start the game.");
			return;
		}

		fmt::println("Joining {} at {}:{}", playerName, serverHost, serverPort);
		joining = true;
		midgard->join(playerName);
	}

	void Application::onKeyCallback(GLFWwindow* windowParam, int key, int scancode, int action, int mods) noexcept {
		// L-key AI stays local and must not move the server hero.
		if (!midgard || !midgard->isConnected()) {
			aiSystem->onKeyCallback(windowParam, key, scancode, action, mods);
		}
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
				requestEndTurn();
			}

			// finish quest once requirements met
			currentQuestId = gameController->getQuestsSystem()->getCurrentShowingQuestId();
			if (const Quest* showingQuest = gameController->getQuestsSystem()->getQuestById(currentQuestId);
				showingQuest && showingQuest->state == QuestState::Completed) {
				if (midgard && midgard->isConnected()) {
					midgard->claimQuest(currentQuestId);
					gameController->getQuestsSystem()->claimQuest(currentQuestId, gameState->getPlayer(gameState->getViewerPlayerId()), gameState.get());
				} else {
					gameController->claimQuestReward(currentQuestId);
				}
			}

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

	void Application::presentHazardState(size_t playerId, bool hadHazardBefore, const std::string& previousHazardName) noexcept {
		Player* player = gameState->getPlayer(playerId);
		const bool hasHazard = player && player->hasActiveHazard();

		auto setHeroAnimation = [&](Hero::AnimationType type) {
			if (registry->animations.entities.empty()) {
				return;
			}
			Entity hero = registry->animations.entities.front();
			if (!registry->animations.has(hero)) {
				return;
			}
			auto& animComp = registry->animations.get(hero);
			if (animComp.currentType != type) {
				animComp.currentType = type;
				animComp.anim.setCurrentFrameIndex(0);
			}
		};

		if (hasHazard) {
			const auto hazard = *player->getActiveHazard();
			const auto& hazardDefinition = HazardDB::getDefinition(hazard.type);
			if (hazard.turnsLeft == hazardDefinition.defaultRoundDuration) {
				fmt::println("[Hazard] {} encountered. It is active for {} turns", hazardDefinition.name, hazard.turnsLeft);
				render.eventPresentationSystem.presentEvent(
					"You encountered a hazard",
					fmt::format(
						"A {} is preventing you\n"
						"from moving for {} turns.\n"
						"Would you like to overcome the\n"
						"encounter by paying {} {} or wait?",
						hazardDefinition.name,
						hazard.turnsLeft,
						hazardDefinition.skipCost * hazard.turnsLeft,
						hazardDefinition.skipRessourceStr),
					{"Pay ressources", "Wait"},
					HazardDB::getEvent(hazardDefinition.hazardType));
				setHeroAnimation(Hero::AnimationType::Attack);
			} else {
				fmt::println("[Hazard] {} encounter ongoing. It is still active for {} turns", hazardDefinition.name, hazard.turnsLeft);
				render.renderNotificationSystem.showNotification(
					"Ongoing hazard",
					fmt::format(
						"A {} is still preventing you from moving for {} turns\n"
						"Would you like to overcome the encounter by paying {} {} or wait?",
						hazardDefinition.name,
						hazard.turnsLeft,
						hazardDefinition.skipCost * hazard.turnsLeft,
						hazardDefinition.skipRessourceStr),
					{"Pay ressources", "Wait"});
			}
			return;
		}

		if (hadHazardBefore) {
			fmt::println("[Hazard] {} encounter ended", previousHazardName);
			render.renderNotificationSystem.showNotification(
				"You overcame the hazard",
				fmt::format("Your encounter with the {} ended", previousHazardName),
				{"Continue"});
			setHeroAnimation(Hero::AnimationType::Idle);
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
		Player* player = this->gameState->getPlayer(0);
		movementSystem->setTarget(randomTileID, hero, player);
		if (player) {
			auto domainHero = std::make_shared<Hero>(static_cast<size_t>(randomTileID), startPosition, "", 3);
			player->setHero(domainHero);
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
				static_cast<float>(window->getWindowExtent().y) - mouseY};


			// Check if any Notification buttons were pressed
			std::string pressedButton = render.renderNotificationSystem.onMouseButton(mouse, button, action);
			// If any button was pressed continue
			if (!pressedButton.empty()) {
				std::cout << "Button: " << pressedButton << " was pressed" << std::endl;

				// finish quest once requirements met
				if (pressedButton == "Claim") {
					int currentId = gameController->getQuestsSystem()->getCurrentShowingQuestId();
					if (midgard && midgard->isConnected()) {
						midgard->claimQuest(currentId);
						gameController->getQuestsSystem()->claimQuest(currentId, gameState->getPlayer(gameState->getViewerPlayerId()), gameState.get());
					} else {
						gameController->claimQuestReward(currentId);
					}
				}

				// TODO: add actions for button pressed in notifications
				if (pressedButton == "Wood" || pressedButton == "Stone" ||
					pressedButton == "Clay" || pressedButton == "Wool" || pressedButton == "Grain") {
					if (!midgard || !midgard->isConnected()) {
						tradingSystem.handleOptionClicked(pressedButton);
					}
					if(world.getShowTrade()){
						world.setShowTrade(false);
					}
				}
				if (pressedButton == "Pay ressources") {
					if (midgard && midgard->isConnected()) {
						midgard->payHazard();
					}
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
				if (pressedButton == "Reset tutorial") {
					gameState->resetTutorial();
				}
				return; // notification clicked -> no further actions (including movement) for now
			}
			if (render.renderNotificationSystem.isActive() || render.eventPresentationSystem.currentEvent) {
				TutorialStep* step = this->gameState->getCurrentTutorialStep();
				if (step && step->id == TutorialStepId::WELCOME) {
					this->gameState->completeCurrentTutorialStep();
				}
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
					requestEndTurn();
					return;
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
						} else if (SideHudButton == "HeroPoints") {
							onKeyCallback(windowParam, GLFW_KEY_V, 0, GLFW_PRESS, 0);
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
						auto* step = this->gameState->getCurrentTutorialStep();
						if (step && step->id == TutorialStepId::SETTLEMENT_MENU) {
							this->gameState->completeCurrentTutorialStep();
						}
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

						const size_t playerId = gameState->getViewerPlayerId();
						if (!isViewerTurn()) {
							return;
						}
						fmt::println("Current player ID: {}", playerId);

						if (this->world.isSettlementPreviewActive) {
							fmt::println("Checking if player can build settlement at world position {},{}", worldPos.x, worldPos.y);
							// Find closest vertex for settlement placement
							auto vertexIdOpt = WorldNodeMapper::findClosestVertexToWorldPos(worldPos, map);
							if (vertexIdOpt.has_value()) {
								fmt::println("Closest vertex found at {}", vertexIdOpt.value());
								size_t vertexId = vertexIdOpt.value();
								if (this->gameController->canBuildSettlement(playerId, vertexId)) { // validate player can build settlement
									fmt::println("Player can build settlement at vertex {}", vertexId);
									const auto settlementCost = this->gameState->getCurrentSettlementCost();
									if (!this->gameController->canAfford(playerId, settlementCost)) {
										render.renderNotificationSystem.showNotification(
											"You don't have enough ressources!",
											"You need more ressources to build this.\nPress 'C' to check for ressource cost.",
											{"Okay"});
									} else if (midgard && midgard->isConnected()) {
										midgard->buildSettlement(vertexId);
										this->world.isSettlementPreviewActive = false;
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

								if (gameController->canBuildRoad(playerId, edgeId)) { // validate player can build road
									fmt::println("Player can build road at edge {}", edgeId);
									const auto roadCost = this->gameState->getCurrentRoadCost();
									if (!gameController->canAfford(playerId, roadCost)) {
										render.renderNotificationSystem.showNotification(
											"You don't have enough ressources!",
											"You need more ressources to build this.\nPress 'C' to check for ressource cost.",
											{"Okay"});
									} else if (midgard && midgard->isConnected()) {
										midgard->buildRoad(edgeId);
										this->world.isRoadPreviewActive = false;
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

				if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && !aiSystem->isAiActive()) {
					glm::vec2 mouseCoords = glm::vec2(mouseX, mouseY);
					auto extent = this->window->getWindowExtent();

					auto tileId = render.renderTilesSystem.getTileIdAtPosition(mouseCoords.x, extent.y - mouseCoords.y);
					auto mapId = render.renderTilesSystem.tileIdToMapId(tileId);
					fmt::println("Picked: TileId {} / MapId {} at mouse ({}, {})", tileId, mapId, mouseCoords.x, mouseCoords.y);

					if (mapId >= 0 && !movementSystem->isEntityMoving()) {
						if (!isViewerTurn()) {
							return;
						}
						//  TODO: For multiplayer use hero of active player
						Entity hero = registry->animations.entities.front();
						Player* player = this->gameState->getPlayer(this->gameState->getViewerPlayerId());
						movementSystem->setTarget(mapId, hero, player);
						auto path = movementSystem->getCurrentPath();
						render.renderTilesSystem.setPath(movementSystem->getCurrentPath());
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

	void Application::enqueueNet(std::function<void()> fn) noexcept {
		std::lock_guard<std::mutex> lock(*netMutex);
		netQueue.push_back(std::move(fn));
	}

	void Application::drainNet() noexcept {
		std::vector<std::function<void()>> pending;
		{
			std::lock_guard<std::mutex> lock(*netMutex);
			pending.swap(netQueue);
		}
		for (auto& fn : pending) {
			fn();
		}
	}

	void Application::onLobby(const df::bifrost::LobbyState& lobby) noexcept {
		if (!midgard || sessionMapReady || startSent) {
			return;
		}

		bool allReady = !lobby.players.empty();
		for (const auto& player : lobby.players) {
			if (!player.ready) {
				allReady = false;
			}
		}

		if (midgard->isHost() && !configSent) {
			df::bifrost::LobbyConfig config = lobby.config;
			if (pendingMode == 0) {
				config.generationMode = df::bifrost::GenerationMode::INSULAR;
			} else if (pendingMode == 1) {
				config.generationMode = df::bifrost::GenerationMode::PERLIN;
			}
			if (pendingSeed >= 0) {
				config.seed = static_cast<uint32_t>(pendingSeed);
			}
			if (pendingWidth > 0) {
				config.columns = static_cast<uint32_t>(pendingWidth);
			}
			if (pendingHeight > 0) {
				config.rows = static_cast<uint32_t>(pendingHeight);
			}
			config.solo = soloRequested;
			midgard->updateConfig(config);
			configSent = true;
		}

		if (!readySent && (!midgard->isHost() || configSent)) {
			midgard->setReady(true);
			readySent = true;
		}

		if (!midgard->isHost() || !allReady || !readySent || startSent) {
			if (!sessionMapReady) {
				if (soloRequested) {
					configMenu.setStatus("");
				} else if (midgard->isHost()) {
					configMenu.setStatus("Waiting for players (" + std::to_string(lobby.players.size()) + "). At least 2 must join.");
				} else {
					configMenu.setStatus("Waiting for the host to start.");
				}
			}
			return;
		}
		const size_t readyPlayers = lobby.players.size();
		if (!soloRequested && readyPlayers < 2) {
			configMenu.setStatus("Waiting for players (" + std::to_string(readyPlayers) + "). At least 2 must join.");
			return;
		}
		configMenu.setStatus("");
		midgard->startGame();
		startSent = true;
	}

	void Application::placeHeroFromServer(bool force) noexcept {
		if (!force && movementSystem->getMovementState()) {
			return;
		}
		const size_t playerId = midgard && midgard->getPlayerId() ? *midgard->getPlayerId() : 0;
		Player* player = gameState->getPlayer(playerId);
		if (!player || !player->getHero() || registry->animations.entities.empty()) {
			return;
		}

		Entity hero = registry->animations.entities.front();
		const size_t tileId = player->getHero()->getTileID();
		if (!force && registry->tileID.has(hero) && registry->tileID.get(hero) == tileId) {
			return;
		}

		const glm::vec2 position = movementSystem->getTileWorldPosition(tileId);
		if (registry->positions.has(hero)) {
			registry->positions.get(hero) = position;
		} else {
			registry->positions.emplace(hero, position);
		}
		if (registry->tileID.has(hero)) {
			registry->tileID.get(hero) = tileId;
		} else {
			registry->tileID.emplace(hero, tileId);
		}
		movementSystem->setTarget(tileId, hero, player);
		heroPlaced = true;
	}

	void Application::onAuthoritativeState(const nlohmann::json& state) noexcept {
		const size_t playerId = midgard && midgard->getPlayerId() ? *midgard->getPlayerId() : 0;
		gameState->setViewerPlayerId(playerId);
		const bool alreadyPlaying = sessionMapReady;
		bool hadHazard = false;
		std::string previousName;
		int previousTurns = -1;
		int previousType = -1;
		if (Player* before = gameState->getPlayer(playerId)) {
			if (before->hasActiveHazard()) {
				hadHazard = true;
				const auto hazard = *before->getActiveHazard();
				previousTurns = hazard.turnsLeft;
				previousType = static_cast<int>(hazard.type);
				previousName = HazardDB::getDefinition(hazard.type).name;
			}
		}

		if (!sessionMapReady) {
			reset();
		}

		gameState->applyAuthoritativeSnapshot(state);
		if (state.contains("quests")) {
			if (QuestsSystem* quests = gameController->getQuestsSystem()) {
				quests->bindPlayer(gameState->getViewerPlayerId());
				quests->applyAuthoritative(state["quests"]);
			}
		}
		sessionMapReady = true;
		placeHeroFromServer(!alreadyPlaying);

		if (const auto result = render.renderTilesSystem.updateMap(); result.isErr()) {
			std::cerr << result.unwrapErr() << std::endl;
		}
		render.renderHeroSystem.updateDimensionsFromMap();
		render.renderWeatherSystem.syncFromGameState();

		if (!tradingReady) {
			if (Player* player = gameState->getPlayer(playerId)) {
				tradingSystem.init(&render.getRenderNotificationSystem(), player);
			}
			world.setTradeCallback([this]() {
				tradingSystem.startTrading();
			});
			tradingReady = true;
		}

		if (render.renderSettlementMenuSystem.isActive()) {
			render.renderSettlementMenuSystem.showMenu(selectedSettlementId);
		}

		bool hasHazard = false;
		int turns = -1;
		int type = -1;
		if (Player* after = gameState->getPlayer(playerId)) {
			if (after->hasActiveHazard()) {
				hasHazard = true;
				const auto hazard = *after->getActiveHazard();
				turns = hazard.turnsLeft;
				type = static_cast<int>(hazard.type);
			}
		}

		if (alreadyPlaying && (hadHazard != hasHazard || previousTurns != turns || previousType != type)) {
			hazardPresentationPending = true;
			hazardHadBefore = hadHazard;
			hazardPreviousName = previousName;
			hazardPlayerId = playerId;
		}

		gameState->setPhase(types::GamePhase::PLAY);
	}

	void Application::onActionResult(bool success, const std::optional<df::bifrost::ErrorInfo>& error) noexcept {
		if (success || !error) {
			return;
		}
		if (!sessionMapReady) {
			fmt::println(stderr, "Could not join: {}", error->message);
			joining = false;
			return;
		}
		if (error->code == df::bifrost::ErrorCode::INSUFFICIENT_RESOURCES) {
			const bool hazard = error->message.find("overcome the hazard") != std::string::npos;
			render.renderNotificationSystem.showNotification(
				hazard ? "Not enough ressources" : "You don't have enough ressources!",
				error->message,
				{hazard ? "Continue" : "Okay"});
			return;
		}
		if (error->code == df::bifrost::ErrorCode::INVALID_ACTION &&
			error->message.find("move") != std::string::npos) {
			movementSystem->cancelMovement();
			render.renderTilesSystem.setPath({});
			render.renderTilesSystem.setSelectedTile(-1);
		}
	}

	bool Application::isViewerTurn() const noexcept {
		return gameState->getViewerPlayerId() == gameState->getCurrentPlayerId();
	}

	void Application::requestEndTurn() noexcept {
		if (!midgard || !midgard->isConnected() || !sessionMapReady) {
			return;
		}
		if (gameState->isGameOver() || movementSystem->getMovementState() || render.renderNotificationSystem.isActive()) {
			return;
		}
		if (render.eventPresentationSystem.currentEvent) {
			return;
		}
		if (!isViewerTurn()) {
			return;
		}

		const size_t playerId = gameState->getCurrentPlayerId();
		Player* current = gameState->getPlayer(playerId);
		if (!current || !current->hasActiveHazard()) {
			const size_t target = movementSystem->getTargetTileId();
			const bool differentTile = current && current->getHero() && current->getHero()->getTileID() != target;
			if (differentTile && gameController->canMoveHeroToTile(playerId, target)) {
				midgard->moveHero(target);
				movementSystem->toggleMovementState();
				fmt::println("Hero destination: {},{}", movementSystem->getTargetPosition().x, movementSystem->getTargetPosition().y);
			} else if (differentTile) {
				movementSystem->cancelMovement();
				render.renderTilesSystem.setPath({});
				render.renderTilesSystem.setSelectedTile(-1);
			}
		}
		midgard->endTurn();
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
