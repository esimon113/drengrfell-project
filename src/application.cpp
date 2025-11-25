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

			df::AnimationSystem::update(registry, delta_time);
			render.step(delta_time);

			window->swapBuffers();
		}
	}

	void Application::reset() noexcept {
		registry->clear(); // remove all components

		// initialize the player
		Entity hero = registry->createEntity();
		registry->positions.emplace(hero, glm::vec2(0.5f, 1.0f));
		registry->scales.emplace(hero, glm::vec2(1.f, 1.f));
		registry->collisionRadius.emplace(hero, 0.5f);
		
		std::vector<std::string> idleFrames = {
			"assets/textures/hero/idle/idle_0.png",
			"assets/textures/hero/idle/idle_1.png",
			"assets/textures/hero/idle/idle_2.png",
			"assets/textures/hero/idle/idle_3.png",
			"assets/textures/hero/idle/idle_4.png",
		};

		Animation idleAnim(idleFrames, 0.4f, true); // 1 picture for 0.4 seconds
		registry->animations.emplace(hero, AnimationComponent{ idleAnim });

		render.reset();
	}

	void Application::onResizeCallback(GLFWwindow* windowParam, int width, int height) noexcept {
		render.onResizeCallback(windowParam, width, height);
	}
}
