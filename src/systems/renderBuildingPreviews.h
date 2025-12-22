#pragma once

#include "gamestate.h"
#include "renderCommon.h"
#include <registry.h>
#include <window.h>
#include <utils/shader.h>
#include <utils/texture.h>
#include "framebuffer.h"



namespace df {
	class RenderBuildingPreviewsSystem {
		public:
			RenderBuildingPreviewsSystem() = default;
			~RenderBuildingPreviewsSystem() = default;

			static RenderBuildingPreviewsSystem init(Window* window, Registry* registry, std::shared_ptr<GameState> gameState) noexcept;
			void deinit() noexcept;
			void step(float dt) noexcept;
			void reset() noexcept;

			void renderPreviews(float time = 0.0f) noexcept;

			void updateViewport(const glm::uvec2& origin, const glm::uvec2& size) noexcept {
				this->viewport.origin = origin;
				this->viewport.size = size;
			}


		private:
			const glm::mat4 calculateProjection(const Camera& cam) const;

			Registry* registry;
			Window* window;
			std::shared_ptr<GameState> gamestate;

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
