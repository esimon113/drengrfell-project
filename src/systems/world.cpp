#include "world.h"



namespace df {
	WorldSystem WorldSystem::init(Window* window, Registry *registry, AudioSystem *audioEngine) noexcept {
		WorldSystem self;

		self.window = window;
		self.registry = registry;
		self.audioEngine = audioEngine;

		self.randomEngine = std::default_random_engine(std::random_device()());

		self.m_reset = true;

		return self;
	}


	void WorldSystem::deinit() noexcept {}


	void WorldSystem::reset() noexcept {
		score = 0;
		m_reset = false;
	}


	void WorldSystem::step(const float delta) noexcept {
		//std::string title = fmt::format("Score: {} - FPS: {:.2f} ({:.2f} ms)", score, 1/delta, 1000 * delta);
		//window->setTitle(title.c_str());

		Camera& cam = registry->cameras.get(registry->getCamera());
		CameraInput& input = registry->cameraInputs.get(registry->getCamera());

		if (input.up)    cam.position.y += cam.scrollSpeed * delta;
		if (input.down)  cam.position.y -= cam.scrollSpeed * delta;
		if (input.left)  cam.position.x -= cam.scrollSpeed * delta;
		if (input.right) cam.position.x += cam.scrollSpeed * delta;

	}


	void WorldSystem::onKeyCallback(GLFWwindow* /* window */, int key, int /* scancode */, int action, int /* mods */) noexcept {
		CameraInput& input = registry->cameraInputs.get(registry->getCamera());
		switch (action) {
			case GLFW_PRESS:
				switch (key) {
					case GLFW_KEY_R: // pressing the 'r' key triggers a reset of the game
						m_reset = true;
						break;
					case GLFW_KEY_W:
						fmt::println("W pressed");
						input.up = true;
						break;
					case GLFW_KEY_A:
						fmt::println("A pressed");
						input.left = true;
						break;
					case GLFW_KEY_S:
						fmt::println("S pressed");
						input.down = true;
						break;
					case GLFW_KEY_D:
						fmt::println("D pressed");
						input.right = true;
						break;
					default:
						break;
				}
				break;

			case GLFW_RELEASE:
				switch (key) {
					case GLFW_KEY_W:
						input.up = false;
						break;
					case GLFW_KEY_A:
						input.left = false;
						break;
					case GLFW_KEY_S:
						input.down = false;
						break;
					case GLFW_KEY_D:
						input.right = false;
						break;
				}
				break;

			case GLFW_REPEAT:
			default:
				break;
		}
	}
}
