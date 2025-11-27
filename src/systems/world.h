#pragma once

#include <registry.h>
#include <window.h>
#include <systems/audio.h>



namespace df {
	class WorldSystem {
		public:
			static WorldSystem init(Window* window, Registry* registry, AudioSystem* audioEngine) noexcept;
			void deinit() noexcept;

			void step(const float delta) noexcept;
			void reset() noexcept;

			inline bool shouldReset() noexcept { return m_reset; }

			void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;
			void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) noexcept;
			void onScrollCallback(GLFWwindow* window, double xoffset, double yoffset) noexcept;

			// for rendering building previews on selection
			bool isSettlementPreviewActive = false;
			bool isRoadPreviewActive = false;


		private:
			static constexpr size_t MAX_EAGLES = 15;
			static constexpr size_t MAX_BUGS = 5;

			Window* window;
			Registry* registry;
			AudioSystem* audioEngine;

			size_t score;

			bool m_reset;

			std::default_random_engine randomEngine;
			std::uniform_real_distribution<float> uniformDistribution;
	};
}
