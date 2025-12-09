#pragma once

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

			glm::vec2 screenToWorldCoordinates(const glm::vec2& screenPos) const noexcept;
			static glm::vec2 calculateWorldDimensions(int columns = 10.0f, int rows = 10.0f) noexcept;

			// REMOVE this. step() should be the only method with "actual" rendering responsibility
			void renderSettlementPreview(const glm::vec2& worldPosition, bool active, float time = 0.0f) noexcept;
			void renderRoadPreview(const glm::vec2& worldPosition, bool active, float time = 0.0f) noexcept;

		private:
			Registry* registry;
			Window* window;

			Framebuffer intermediateFramebuffer;

			GLuint m_quad_vao;
			GLuint m_quad_ebo;

			struct {
				glm::uvec2 m_origin;
				glm::uvec2 m_size;
			} m_viewport;

			RenderTilesSystem renderTilesSystem;
			RenderHeroSystem renderHeroSystem;
			RenderBuildingsSystem renderBuildingsSystem;
	};
}
