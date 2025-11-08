#pragma once

#include <common.h>
#include <tiny_ecs.hpp>



namespace df {
	struct Player {};



	// NOTE: For your own project you may want to move the registry to a different file, as it grows in size.
	//	 Depending on the requirements of your projects it may also make sense to have multiple registries.
	class Registry {
		public:
			static Registry* init() noexcept;
			void clear() noexcept;
			void clear(const Entity entity) noexcept;

			ComponentContainer<glm::vec2> positions;
			ComponentContainer<glm::vec2> velocities;
			ComponentContainer<glm::vec2> scales;
			ComponentContainer<float> angles;

			ComponentContainer<Player> players;

			ComponentContainer<float> collisionRadius;

			ComponentContainer<glm::vec3> colors;

			inline Entity getPlayer() noexcept { return player; }
			inline float& getScreenDarkness() noexcept { return screenDarkness; }


		private:
			std::array<ContainerInterface*, 11> containers;

			Entity player;
			float screenDarkness;
	};
} // namespace df
