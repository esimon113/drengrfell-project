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

		if (gl3wInit()) {
			fmt::println(stderr, "Failed to initialize OpenGL context");
			self.window->deinit();
			glfwTerminate();
			return ::std::nullopt;
		}
		fmt::println("Loaded OpenGL {} & GLSL {}", (char*)glGetString(GL_VERSION), (char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

		self.registry = Registry::init();
		// Create GameState
		GameState newGameState(self.registry);
		self.gameState = std::move(newGameState);
		self.world = WorldSystem::init(self.window, self.registry, nullptr);	// nullptr used to be self.audioEngine, as long as that is not yet needed, it is set to nullptr
		// self.physics = PhysicsSystem::init(self.registry, self.audioEngine);
		self.render = RenderSystem::init(self.window, self.registry);

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
			glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glfwPollEvents();

			float time = static_cast<float>(glfwGetTime());
			delta_time = time - last_time;
			last_time = time;

			world.step(delta_time);
			// physics.step(delta_time);
			// physics.handleCollisions(delta_time);
			df::AnimationSystem::update(registry, delta_time);
			render.step(delta_time);

			// Render previews (only one at a time)
			if (this->world.isSettlementPreviewActive) {
				glm::vec2 cursorPos = window->getCursorPosition();
				glm::vec2 worldPos = screenToWorldCoordinates(cursorPos, render.getViewport());
				this->render.renderSettlementPreview(worldPos, true, time);
			}
			else if (this->world.isRoadPreviewActive) {
				glm::vec2 cursorPos = window->getCursorPosition();
				glm::vec2 worldPos = screenToWorldCoordinates(cursorPos, render.getViewport());
				this->render.renderRoadPreview(worldPos, true, time);
			}

			window->swapBuffers();
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
