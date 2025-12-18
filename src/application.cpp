#include "application.h"
#include "GL/gl3w.h"
#include "GL/glcorearb.h"
#include "hero.h"
#include "animationSystem.h"
#include "types.h"
#include "core/camera.h"
#include <glm/gtc/matrix_transform.hpp>
// test for entityMovement
#include "entityMovement.h"

#include <iostream>
#include <fstream>

#include "worldGenerator.h"



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

		self.registry = Registry::init();
		self.gameState = std::make_shared<GameState>(self.registry);
		self.world = WorldSystem::init(self.window, self.registry, nullptr, *self.gameState);	// nullptr used to be self.audioEngine, as long as that is not yet needed, it is set to nullptr
		// self.physics = PhysicsSystem::init(self.registry, self.audioEngine);
		self.render = RenderSystem::init(self.window, self.registry, *self.gameState);
		// Create main menu
		self.mainMenu.init(self.window);
		// for testing hero movement until we have a triggerpoint
		self.movementSystem = EntityMovementSystem::init(self.registry, *self.gameState);
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
					df::AnimationSystem::update(registry, delta_time);
					window->makeContextCurrent();
					glClearColor(0.5f,0.5f,0.5f,1.0f);
					glClear(GL_COLOR_BUFFER_BIT);
					
					render.step(delta_time);
					// ------- only here for testing until we have a triggerpoint for the movement-----------------------------------------------------
					if (world.isTestMovementActive()) {
						if (!registry->animations.entities.empty()) {

							Entity hero = registry->animations.entities.front();
							glm::vec2 targetPos = glm::vec2(6,6);
							movementSystem.moveEntityTo(hero, targetPos, delta_time);
						}
						else {
							fmt::println("No hero entity available!");
						}
					}
					// ------------------------------------------------------------
					// Render previews (only one at a time)
					auto renderBuildingsSystem = this->render.renderBuildingsSystem;

					if (this->world.isSettlementPreviewActive) {
						glm::vec2 cursorPos = window->getCursorPosition();
						glm::vec2 worldPos = screenToWorldCoordinates(cursorPos, renderBuildingsSystem.getViewport());
						renderBuildingsSystem.renderSettlementPreview(worldPos, true, time);
					}
					else if (this->world.isRoadPreviewActive) {
						glm::vec2 cursorPos = window->getCursorPosition();
						glm::vec2 worldPos = screenToWorldCoordinates(cursorPos, renderBuildingsSystem.getViewport());
						renderBuildingsSystem.renderRoadPreview(worldPos, true, time);
					}
				}
					break;
				case types::GamePhase::END:
					break;
			}


			window->swapBuffers();
		}
	}

	void Application::reset() noexcept {
		registry->clear();

		// initialize the player
		registry->players.emplace(registry->getPlayer());
		registry->positions.emplace(registry->getPlayer(), 0.5f, 0.5f);
		registry->velocities.emplace(registry->getPlayer(), 0, 0);
		registry->scales.emplace(registry->getPlayer(), -0.1f, 0.1f);
		registry->angles.emplace(registry->getPlayer(), 0.f);
		registry->collisionRadius.emplace(registry->getPlayer(), 0.1f);

		registry->getScreenDarkness() = 1.f;

		// reset systems
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


		// generate map with the WorldGeneratorConfig
		if (const auto worldGenConfResult = WorldGeneratorConfig::deserialize(); worldGenConfResult.isErr()) {
			std::cerr << worldGenConfResult.unwrapErr() << std::endl;
			gameState->getMap().regenerate();
		}
		else {
			gameState->getMap().regenerate(worldGenConfResult.unwrap<>());
		}
		{
			Player player{};
			player.addResources(types::TileType::FOREST, 100);				// give player 100 wood
			player.addResources(types::TileType::MOUNTAIN, 100);			// give player 100 stone
			player.addResources(types::TileType::FIELD, 50);				// give player 50 grain
			gameState->addPlayer(player);
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
		}
		if (const auto result = render.renderTilesSystem.updateMap(); result.isErr()) {
			std::cerr << result.unwrapErr() << std::endl;
		}
		render.renderHeroSystem.updateDimensionsFromMap();

		gameState->initTutorial();	// Init the Tutorial
		gameState->setPhase(types::GamePhase::PLAY);
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

			if (render.renderHudSystem.onMouseButton(mouse, button, action))
				return;

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
		types::GamePhase gamePhase = gameState->getPhase();

		switch (gamePhase) {
		case types::GamePhase::START:
			mainMenu.onResizeCallback(windowParam, width, height);
			render.onResizeCallback(windowParam, width, height);
			render.renderHudSystem.onResizeCallback(windowParam, width, height);
			configMenu.onResizeCallback(windowParam, width, height);
			break;
		case types::GamePhase::CONFIG:
			configMenu.onResizeCallback(windowParam, width, height);
			break;
		case types::GamePhase::PLAY:
			render.onResizeCallback(windowParam, width, height);
			render.renderHudSystem.onResizeCallback(windowParam, width, height);
			break;
		case types::GamePhase::END:
			break;
		}
	}
}