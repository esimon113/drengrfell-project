#include "application.h"
#include "GL/gl3w.h"
#include "GL/glcorearb.h"

#include <iostream>


namespace df {
	static void glfwErrorCallback(int error, const char* description) {
		fmt::println(stderr, "[GLFW Error {}]: {}", error, description);
	}


	// #if !defined(__APPLE__)
	// static void glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei , const GLchar* message, const void*) {
	// 	(void)source;
	// 	(void)type;
	// 	(void)id;
	// 	(void)severity;
	// 	if (type == GL_DEBUG_TYPE_OTHER) return;
	// 	fmt::println(stderr, "[GL DEBUG MESSAGE]: {}", message);
	// }
	// #endif


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

	// #if !defined(__APPLE__)
	// 	glDebugMessageCallback(glDebugCallback, nullptr);
	// #endif

		// self.audioEngine = new AudioSystem;
		// *self.audioEngine = AudioSystem::init();

		self.registry = Registry::init();

		// self.world = WorldSystem::init(self.window, self.registry, self.audioEngine);
		// self.physics = PhysicsSystem::init(self.registry, self.audioEngine);
		self.render = RenderSystem::init(self.window, self.registry);

		return self;
	}


	void Application::deinit() noexcept {
		// world.deinit();
		// physics.deinit();
		render.deinit();
		// audioEngine->deinit();

		delete registry;
		window->deinit();
		delete window;
		glfwTerminate();
	}


	void Application::run() noexcept {
		// ma_sound* music = audioEngine->getBackgroundMusic();
		// ma_sound_set_looping(music, MA_TRUE);
		// ma_sound_start(music);


		if (!this->window || !this->window->getHandle()) {
			std::cerr << "Invalid window or GLFWwindow handle!" << std::endl;
			return;
		}

		window->setResizeCallback([&](GLFWwindow* window, int width, int height) -> void {
				onResizeCallback(window, width, height);
				});
		// window->setKeyCallback([&](GLFWwindow* window, int key, int scancode, int action, int mods) -> void {
		// 		onKeyCallback(window, key, scancode, action, mods);
		// 		});

		std::cout << "	- Set windows->Callbacks" << std::endl;
		float delta_time = 0;
		float last_time = static_cast<float>(glfwGetTime());

		std::cout << "	- Try setting clear color" << std::endl;
		glClearColor(0, 0, 0, 1);

		std::cout << "	- Enter loop: setting background to gray..." << std::endl;
		while (!window->shouldClose()) {

			glClearColor(0.5, 0.5, 0.5, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			glfwPollEvents();

			// if (world.shouldReset()) reset();

			float time = static_cast<float>(glfwGetTime());
			delta_time = time - last_time;
			last_time = time;

			// world.step(delta_time);
			// physics.step(delta_time);
			// physics.handleCollisions(delta_time);
			render.step(delta_time);

			window->swapBuffers();
		}
	}


	void Application::reset() noexcept {
		// reset all game state
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
		// world.reset();
		// physics.reset();
		render.reset();
	}


	// void Application::onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept {
	// 	world.onKeyCallback(window, key, scancode, action, mods);
	// }


	void Application::onResizeCallback(GLFWwindow* window, int width, int height) noexcept {
		render.onResizeCallback(window, width, height);
	}
}
