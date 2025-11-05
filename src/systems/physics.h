#pragma once

#include <common.h>
#include <registry.h>
#include <systems/audio.h>



namespace df {
	class PhysicsSystem {
		public:
			static PhysicsSystem init(Registry* registry, AudioSystem* audio_engine) noexcept;
			void deinit() noexcept;

			void step(const float delta) noexcept;
			void handleCollisions(const float delta) noexcept;
			void reset() noexcept;

		private:
			Registry* m_registry;
			AudioSystem* m_audio_engine;

			struct Collision {
				Entity m_first; Entity m_second;
				bool operator==(const Collision& other) const {
					return (m_first == other.m_first && m_second == other.m_second)
						|| (m_first == other.m_second && m_second == other.m_first);
				}
			};
			std::vector<Collision> m_collisions;
	};
}
