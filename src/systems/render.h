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

namespace df {
	class RenderSystem {
		public:
			RenderSystem() = default;
			~RenderSystem() = default;

			static RenderSystem init(Window* window, Registry* registry) noexcept;
			void deinit() noexcept;

			void step(float dt) noexcept;
			void reset() noexcept;

			void onResizeCallback(GLFWwindow* window, int width, int height) noexcept;

			// Temporarily. REMOVE
			[[nodiscard]] RenderBuildingsSystem& getRenderBuildingsSystem() noexcept {
				return this->renderBuildingsSystem;
			}

		private:
			Registry* registry = nullptr;
			Window* window = nullptr;

			Viewport viewport = Viewport();
			Framebuffer intermediateFramebuffer;

			RenderTilesSystem renderTilesSystem;
			RenderHeroSystem renderHeroSystem;
			RenderBuildingsSystem renderBuildingsSystem;
			RenderHudSystem renderHudSysten;
	};
}
