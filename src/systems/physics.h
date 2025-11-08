#pragma once

#include <common.h>
#include <registry.h>
#include <systems/audio.h>



namespace df {
	class PhysicsSystem {
		public:
			static PhysicsSystem init(Registry* registry, AudioSystem* audioEngine) noexcept;
			void deinit() noexcept;

			void step(const float delta) noexcept;
			void handleCollisions(const float delta) noexcept;
			void reset() noexcept;


		private:
			Registry* registry;
			AudioSystem* audioEngine;


			struct Collision {
				Entity first;
				Entity second;

				bool operator==(const Collision& other) const {
					return (first == other.first && second == other.second) || (first == other.second && second == other.first);
				}
			};

			std::vector<Collision> collisions;
	};
}
