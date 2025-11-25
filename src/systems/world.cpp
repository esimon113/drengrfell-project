#include "world.h"
#include "hero.h"


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
		std::string title = fmt::format("Score: {} - FPS: {:.2f} ({:.2f} ms)", score, 1/delta, 1000 * delta);
		window->setTitle(title.c_str());

		
	}


	void WorldSystem::onKeyCallback(GLFWwindow* /* window */, int key, int /* scancode */, int action, int /* mods */) noexcept {
		switch (action) {
			case GLFW_PRESS:
				switch (key) {
					case GLFW_KEY_R: // pressing the 'r' key triggers a reset of the game
						m_reset = true;
						break;
					// TODO: (A2) Handle player movement here
					default:
						break;
				}
				break;

			case GLFW_RELEASE:
				{} break;

			case GLFW_REPEAT:
			default:
				break;
		}
	}
}
