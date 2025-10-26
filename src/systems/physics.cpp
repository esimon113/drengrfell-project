#include "physics.h"
#include <algorithm>

PhysicsSystem PhysicsSystem::init(Registry* registry, AudioSystem* audio_engine) noexcept {
	PhysicsSystem self;
	self.m_registry = registry;
	self.m_audio_engine = audio_engine;
	return self;
}

void PhysicsSystem::deinit() noexcept {}

enum struct EatOrBeEaten {
	eat, eaten, neither
};

EatOrBeEaten eatOrBeEaten(Registry* registry, const Entity first, const Entity second) noexcept {
	if (registry->m_players.has(first)) {
		if (registry->m_eagles.has(second)) {
			if (!registry->m_death_timers.has(first)) {
				registry->m_death_timers.emplace(first, 2.f);
				return EatOrBeEaten::eaten;
			}
		} else if (registry->m_bugs.has(second)) {
			registry->clear(second);
			return EatOrBeEaten::eat;
		}
	}
	return EatOrBeEaten::neither;
}

void PhysicsSystem::step(const float /*delta*/) noexcept {
	// TODO: (A2) Update the angle of the player based on the current cursor position here.
	for (Entity e : m_registry->m_velocities.entities) {
		(void)e;
		// TODO: (A2) Handle updates to position here.
	}

	m_collisions.clear();
	for (Entity e1 : m_registry->m_collision_radius.entities) {
		glm::vec2 position1 = m_registry->m_positions.get(e1);
		float radius1 = m_registry->m_collision_radius.get(e1);

		for (Entity e2 : m_registry->m_collision_radius.entities) {
			if (e2 == e1) continue;
			glm::vec2 position2 = m_registry->m_positions.get(e2);
			float radius2 = m_registry->m_collision_radius.get(e2);
			
			if (glm::distance(position1, position2) <= radius1 + radius2) {
				PhysicsSystem::Collision collision = { e1, e2 };
				auto first = m_collisions.begin();
				auto last = m_collisions.end();
				auto it = std::find(first, last, collision);
				if (it == last) {
					m_collisions.emplace_back(collision);
				}
			}
		}
	}
}

void PhysicsSystem::handleCollisions(const float /*delta*/) noexcept {
	for (auto& collision : m_collisions) {
		auto [first, second] = collision;
		EatOrBeEaten result = eatOrBeEaten(m_registry, first, second);
		if (result == EatOrBeEaten::neither) result = eatOrBeEaten(m_registry, second, first);
		switch (result) {
			case EatOrBeEaten::eat:
				{
					ma_sound_start(m_audio_engine->eatSound());
					// TODO: (A2) Create a new `LightUp` component in application.h and add an instance to the chicken here.
				} break;
			case EatOrBeEaten::eaten:
				{
					ma_sound_start(m_audio_engine->deadSound());
					// TODO: (A2) Change the color and orientation of the chicken on death
				} break;
			case EatOrBeEaten::neither:
				break;
		}
        }
}

void PhysicsSystem::reset() noexcept {}
