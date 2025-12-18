#pragma once

#include "gamestate.h"
#include "renderCommon.h"
#include <registry.h>
#include <window.h>
#include <utils/shader.h>
#include <utils/texture.h>
#include "framebuffer.h"

namespace df {
	class RenderBuildingsSystem {
		public:
			RenderBuildingsSystem() = default;
			~RenderBuildingsSystem() = default;

			static RenderBuildingsSystem init(Window* window, Registry* registry, GameState& gameState) noexcept;
			void deinit() noexcept;
			void step(float dt) noexcept;
			void reset() noexcept;

			void renderSettlementPreview(const glm::vec2& worldPosition, bool active, float time = 0.0f) noexcept;
			void renderRoadPreview(const glm::vec2& worldPosition, bool active, float time = 0.0f) noexcept;


			void updateViewport(const glm::uvec2& origin, const glm::uvec2& size) noexcept {
                this->viewport.origin = origin;
                this->viewport.size = size;
            }
			// Temporary. REMOVE this
			[[nodiscard]] Viewport getViewport() const noexcept {
				return this->viewport;
			}
		private:
			Registry* registry;
			Window* window;
			GameState* gamestate;

			Framebuffer intermediateFramebuffer;

			Shader buildingHoverShader;
			Shader buildingShadowShader;
			Texture settlementTexture;
			Texture roadPreviewTexture;

			GLuint m_quad_vao;
			GLuint m_quad_ebo;

			Viewport viewport;
	};
}
