#pragma once

#include "renderCommon.h"

#include <registry.h>
#include <window.h>
#include <utils/shader.h>
#include <utils/framebuffer.h>

#include "renderTiles.h"
#include "renderBuildings.h"
#include "renderHero.h"

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

			// REMOVE this. step() should be the only method with "actual" rendering responsibility
			void renderSettlementPreview(const glm::vec2& worldPosition, bool active, float time = 0.0f) noexcept;
			void renderRoadPreview(const glm::vec2& worldPosition, bool active, float time = 0.0f) noexcept;

			Viewport getViewport() const noexcept {
				return this->viewport;
			}

		private:
			Registry* registry;
			Window* window;

			Framebuffer intermediateFramebuffer;

			GLuint m_quad_vao;
			GLuint m_quad_ebo;

			Viewport viewport;

			RenderTilesSystem renderTilesSystem;
			RenderHeroSystem renderHeroSystem;
			RenderBuildingsSystem renderBuildingsSystem;
	};
}
