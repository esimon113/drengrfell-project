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
		std::string title = fmt::format("Score: {} - FPS: {:.2f} ({:.2f} ms)", score, 1/delta, 1000 * delta);
		window->setTitle(title.c_str());

		// remove entites that leave the screen on the bottom side
		for (Entity e : registry->velocities.entities) {
			glm::vec2 position = registry->positions.get(e);
			glm::vec2 scale = glm::vec2(0);

			if (registry->scales.has(e)) {
				scale = registry->scales.get(e);
			}
			else if (registry->collisionRadius.has(e)) {
				scale = glm::vec2(registry->collisionRadius.get(e));
			}

			if (!registry->players.has(e) && position.y + fabs(scale.x/2) < -1) {
				registry->clear(e);
			}
		}
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
