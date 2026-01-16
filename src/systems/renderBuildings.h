#pragma once

#include "framebuffer.h"
#include "gamestate.h"
#include "renderCommon.h"
#include <array>
#include <random>
#include <vector>
#include <registry.h>
#include <utils/shader.h>
#include <utils/texture.h>
#include <window.h>




namespace df {
	class RenderBuildingsSystem {
	  public:
		RenderBuildingsSystem() = default;
		~RenderBuildingsSystem() = default;

		static RenderBuildingsSystem init(Window* window, Registry* registry, std::shared_ptr<GameState> gameState) noexcept;
		void deinit() noexcept;
		void step(float dt) noexcept;
		void reset() noexcept;

		void renderBuildings(float time = 0.0f) noexcept;


		void updateViewport(const glm::uvec2& origin, const glm::uvec2& size) noexcept {
			this->viewport.origin = origin;
			this->viewport.size = size;
		}


	  private:
		const glm::mat4 calculateProjection(const Camera& cam) const;

		struct DustPuff {
			glm::vec2 position;
			float startTime = 0.0f;
			float duration = 0.0f;
			float size = 1.0f;
			float baseAlpha = 0.2f;
		};

		void spawnDustAt(const glm::vec2& worldPos, float time, int count, float baseSize) noexcept;
		void renderDust(float time, const glm::mat4& view, const glm::mat4& projection, const Camera& cam) noexcept;

		Registry* registry;
		Window* window;
		std::shared_ptr<GameState> gamestate;

		Framebuffer intermediateFramebuffer;

		Shader spriteShader; // For placed buildings (no pulsing effect)
		Shader locationHighlightShader; // Reused for dust puffs when placing building
		// different texture for different edge angles:
		Texture roadTextureDiagonalDown;		   // 0, 3
		Texture roadTextureDiagonalUp;			   // 2, 5
		Texture roadTextureVertical;			   // 1, 4
		std::array<Texture, 5> settlementTextures; // wood settlement textures for animation

		GLuint m_quad_vao;
		GLuint m_quad_ebo;

		Viewport viewport;

		std::mt19937 rng;
		std::vector<DustPuff> dustPuffs;
		size_t lastSettlementCount = 0;
		size_t lastRoadCount = 0;
	};
} // namespace df
