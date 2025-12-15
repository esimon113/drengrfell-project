#pragma once

#include "renderCommon.h"

#include <registry.h>
#include <window.h>
#include <utils/shader.h>
#include <utils/framebuffer.h>


#include "renderTiles.h"
#include "renderBuildings.h"
#include "renderHero.h"
#include "renderHud.h"
#include "renderText.h"
#include "renderSnow.h"

namespace df {
	class RenderSystem {
		public:
			RenderSystem() = default;
			~RenderSystem() = default;

			static RenderSystem init(Window* window, Registry* registry, GameState& gameState) noexcept;
			void deinit() noexcept;

			void step(float dt) noexcept;
			void reset() noexcept;

			void onResizeCallback(GLFWwindow* window, int width, int height) noexcept;

			RenderTilesSystem renderTilesSystem;
			RenderHeroSystem renderHeroSystem;
			RenderBuildingsSystem renderBuildingsSystem;
			RenderTextSystem renderTextSystem;
			RenderHudSystem renderHudSystem;
			RenderSnowSystem renderSnowSystem; 

			RenderTextSystem& getRenderTextSystem() noexcept {
				return renderTextSystem;
			}

		private:
			Registry* registry;
			Window* window;

			Viewport viewport = Viewport();
			Framebuffer intermediateFramebuffer;

	};
}
