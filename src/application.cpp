#include "application.h"
#include "GL/gl3w.h"
#include "GL/glcorearb.h"
#include "hero.h"
#include "animationSystem.h"

#include <iostream>

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


		::std::optional<Window*> debug_win_opt = Window::init(400, 400, "Particle Debug View");
		if (!debug_win_opt) {
			// Handle error: deinit the main window before terminating GLFW
			self.window->deinit();
			glfwTerminate();
			return ::std::nullopt;
		}
		self.debugWindow = *debug_win_opt; // Store the pointer
    
    // Make the main window's context current for GL3W initialization
    self.window->makeContextCurrent(); // Assuming Window has this method

		if (gl3wInit()) {
			fmt::println(stderr, "Failed to initialize OpenGL context");
			self.window->deinit();
			glfwTerminate();
			return ::std::nullopt;
		}
		fmt::println("Loaded OpenGL {} & GLSL {}", (char*)glGetString(GL_VERSION), (char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

		self.registry = Registry::init();
		GameState newGameState(self.registry);
		self.gameState = std::move(newGameState);
		
		// Initialize systems for the MAIN window
		self.world = WorldSystem::init(self.window, self.registry, nullptr);
		self.render = RenderSystem::init(self.window, self.registry);
		
		// Initialize RenderSnowSystem for the DEBUG window
		// First, make the DEBUG window's context current! (If RenderSnowSystem::init has OpenGL calls)
		self.debugWindow->makeContextCurrent(); 
		self.renderSnowSystem = RenderSnowSystem::init(self.debugWindow, self.registry);
		
		// IMPORTANT: Switch the context back to the main window for the rest of setup/main loop
		self.window->makeContextCurrent(); 

		return self;
	}

	void Application::deinit() noexcept {
		render.deinit();
		delete registry;
		if (debugWindow) {
			debugWindow->deinit();
			delete debugWindow;
		}
		window->deinit();
		delete window;
		glfwTerminate();
	}

	void Application::run() noexcept {
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



		float delta_time = 0;
		float last_time = static_cast<float>(glfwGetTime());

		glClearColor(0, 0, 0, 1);

		while (!window->shouldClose()) {

			glfwPollEvents();
			float time = static_cast<float>(glfwGetTime());
			delta_time = time - last_time;
			last_time = time;
			
			// --- B. Update Systems (Physics/Logic/Animation) ---
			world.step(delta_time);
			df::AnimationSystem::update(registry, delta_time);

			// --- C. Render MAIN Window (Map, Buildings, Hero) ---
			window->makeContextCurrent();
			glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			
			render.step(delta_time);

			glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glfwPollEvents();

			time = static_cast<float>(glfwGetTime());
			delta_time = time - last_time;
			last_time = time;

			world.step(delta_time);
			// physics.step(delta_time);
			// physics.handleCollisions(delta_time);
			df::AnimationSystem::update(registry, delta_time);
			render.step(delta_time);

			// Render previews (only one at a time)
			auto renderBuildingsSystem = this->render.getRenderBuildingsSystem();

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

			window->swapBuffers();

			if (debugWindow) {
				if (!debugWindow->shouldClose()) {
					debugWindow->makeContextCurrent(); // Activate the particle context
					
					// Clear with a specific color (e.g., black) to distinguish it
					glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
					glClear(GL_COLOR_BUFFER_BIT);
					
					// Now, call the snow system's draw method, passing delta_time
					this->renderSnowSystem.step(delta_time); // <--- ASSUMING RenderSnowSystem has a 'step' or 'draw' method

					// Swap buffers for the DEBUG window
					debugWindow->swapBuffers();
				} else {
					// If the user closed the debug window, clean it up
					debugWindow->deinit();
					delete debugWindow;
					debugWindow = nullptr;
				}
			}
			glfwPollEvents();
		}
	}

	void Application::reset() noexcept {
		registry->clear(); // remove all components

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
		// physics.reset();
		render.reset();
	}


	void Application::onKeyCallback(GLFWwindow* windowParam, int key, int scancode, int action, int mods) noexcept {
		world.onKeyCallback(windowParam, key, scancode, action, mods);
	}

	void Application::onMouseButtonCallback(GLFWwindow* windowParam, int button, int action, int mods) noexcept {
		world.onMouseButtonCallback(windowParam, button, action, mods);
	}

	void Application::onScrollCallback(GLFWwindow* windowParam, double xoffset, double yoffset) noexcept {
		world.onScrollCallback(windowParam, xoffset, yoffset);
	}


	void Application::onResizeCallback(GLFWwindow* windowParam, int width, int height) noexcept {
		render.onResizeCallback(windowParam, width, height);
	}
}
