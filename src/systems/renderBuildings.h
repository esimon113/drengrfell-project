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

			void renderBuildings(float time = 0.0f) noexcept;
			void renderPreviews(float time = 0.0f) noexcept;


			void updateViewport(const glm::uvec2& origin, const glm::uvec2& size) noexcept {
				this->viewport.origin = origin;
				this->viewport.size = size;
			}

			// Temporary. REMOVE this
			[[nodiscard]] Viewport getViewport() const noexcept {
				return this->viewport;
			}


		private:
		const glm::mat4 calculateProjection(const Camera& cam) const;

			Registry* registry;
			Window* window;
			GameState* gamestate;

			Framebuffer intermediateFramebuffer;

			Shader buildingHoverShader;
			Shader buildingShadowShader;
			Shader spriteShader; // For placed buildings (no pulsing effect)
			Texture settlementTexture;
			Texture roadPreviewTexture;
			Texture roadTexture; // For actual roads
			std::array<Texture, 5> settlementTextures; // All 5 viking-wood settlement textures -> will be used for settlement animation

			GLuint m_quad_vao;
			GLuint m_quad_ebo;

			Viewport viewport;
	};
}
