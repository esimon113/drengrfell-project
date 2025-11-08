#include "physics.h"
#include <algorithm>



namespace df {
	PhysicsSystem PhysicsSystem::init(Registry* registry, AudioSystem* audioEngine) noexcept {
		PhysicsSystem self;
		self.registry = registry;
		self.audioEngine = audioEngine;

		return self;
	}


	void PhysicsSystem::deinit() noexcept {}


	void PhysicsSystem::step(const float /*delta*/) noexcept {
		// TODO: (A2) Update the angle of the player based on the current cursor position here.
		for (Entity e : registry->velocities.entities) {
			(void)e;
			// TODO: (A2) Handle updates to position here.
		}

		collisions.clear();

		for (Entity e1 : registry->collisionRadius.entities) {
			glm::vec2 position1 = registry->positions.get(e1);
			float radius1 = registry->collisionRadius.get(e1);

			for (Entity e2 : registry->collisionRadius.entities) {
				if (e2 == e1) continue;

				glm::vec2 position2 = registry->positions.get(e2);
				float radius2 = registry->collisionRadius.get(e2);

				if (glm::distance(position1, position2) <= radius1 + radius2) {
					PhysicsSystem::Collision collision = { e1, e2 };
					auto first = collisions.begin();
					auto last = collisions.end();
					auto it = std::find(first, last, collision);

					if (it == last) {
						collisions.emplace_back(collision);
					}
				}
			}
		}
	}


	void PhysicsSystem::handleCollisions(const float /*delta*/) noexcept {
		// ...
	}


	void PhysicsSystem::reset() noexcept {}
}
