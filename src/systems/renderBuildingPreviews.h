#pragma once

#include "framebuffer.h"
#include "gamestate.h"
#include "renderCommon.h"
#include <array>
#include <registry.h>
#include <utils/shader.h>
#include <utils/texture.h>
#include <window.h>

namespace df {
	class GameController;

	class RenderBuildingPreviewsSystem {
	  public:
		RenderBuildingPreviewsSystem() = default;
		~RenderBuildingPreviewsSystem() = default;

		static RenderBuildingPreviewsSystem init(Window* window, Registry* registry, std::shared_ptr<GameState> gameState, GameController* gameController) noexcept;
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

		void renderLocationHighlights(const glm::mat4& view, const glm::mat4& projection, const Camera& cam, float time) noexcept;

		Registry* registry;
		Window* window;
		std::shared_ptr<GameState> gamestate;
		GameController* gameController;

		Framebuffer intermediateFramebuffer;

		Shader buildingHoverShader;
		Shader buildingShadowShader;
		Shader locationHighlightShader;
		std::array<Texture, 5> settlementTextures; // wood settlement textures for animation
		Texture roadPreviewTexture;

		GLuint m_quad_vao;
		GLuint m_quad_ebo;

		Viewport viewport;

		// Highlight configuration
		static constexpr float baseHighlightRadius = 1.5f; // Base radius in world units
		static constexpr float highlightBaseAlpha = 1.0f;  // Maximum alpha for highlights
	};
} // namespace df
