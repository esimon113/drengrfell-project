#pragma once

#include <registry.h>
#include <window.h>
#include <systems/audio.h>

#include "gamestate.h"


namespace df {
	class WorldSystem {
		public:
			static WorldSystem init(Window* window, Registry* registry, AudioSystem* audioEngine, GameState& gameState) noexcept;
			void deinit() noexcept;

			void step(const float delta) noexcept;
			void reset() noexcept;

			inline bool shouldReset() noexcept { return m_reset; }

			void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;
			void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) noexcept;
			void onScrollCallback(GLFWwindow* window, double xoffset, double yoffset) noexcept;
			void calcNewCameraZoom(double yoffset) noexcept;
			bool isTestMovementActive() const { return heroMovementState; }

			// for rendering building previews on selection
			bool isSettlementPreviewActive = false;
			bool isRoadPreviewActive = false;


		private:
			Window* window;
			Registry* registry;
			AudioSystem* audioEngine;
			GameState* gameState;

			size_t score;
			Entity heroEntity;
			bool m_reset;
			bool heroMovementState = false;

			std::default_random_engine randomEngine;
			std::uniform_real_distribution<float> uniformDistribution;
	};
}
