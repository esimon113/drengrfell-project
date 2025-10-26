#pragma once

#include <common.h>
#include <tiny_ecs.hpp>

struct Player {};
struct Eagle {};
struct Egg {};
struct Bug {};

// NOTE: For your own project you may want to move the registry to a different file, as it grows in size.
//	 Depending on the requirements of your projects it may also make sense to have multiple registries.
class Registry {
	public:
		static Registry* init() noexcept;
		void clear() noexcept;
		void clear(const Entity entity) noexcept;

		ComponentContainer<glm::vec2> m_positions;
		ComponentContainer<glm::vec2> m_velocities;
		ComponentContainer<glm::vec2> m_scales;
		ComponentContainer<float> m_angles;

		ComponentContainer<Player> m_players;
		ComponentContainer<Eagle> m_eagles;
		ComponentContainer<Egg> m_eggs;
		ComponentContainer<Bug> m_bugs;

		ComponentContainer<float> m_death_timers;
		ComponentContainer<float> m_collision_radius;

		ComponentContainer<glm::vec3> m_colors;

		inline Entity player() noexcept { return m_player; }
		inline float& screenDarkness() noexcept { return m_screen_darkness; }

	private:
		std::array<ContainerInterface*, 11> m_containers;

		Entity m_player;
		float m_screen_darkness;
};
