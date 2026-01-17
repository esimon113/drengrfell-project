#pragma once

#include "framebuffer.h"
#include "gamestate.h"
#include "renderCommon.h"
#include <array>
#include <random>
#include <unordered_map>
#include <unordered_set>
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

		glm::vec2 getCursorWorldPos(const Camera& cam) const noexcept;
		
		float distanceToEdge(const Graph& map, const glm::vec2& cursorWorldPos, size_t edgeId) const noexcept;

		size_t findClosestOwnedEdge(const Graph& map,
			const glm::vec2& cursorWorldPos,
			const std::unordered_map<size_t, Entity>& ownedRoadEntitiesByEdge,
			float& outClosestDistance) const noexcept;

		std::unordered_set<size_t> collectConnectedOwnedEdges(const Graph& map,
			size_t startEdgeId,
			const std::unordered_map<size_t, Entity>& ownedRoadEntitiesByEdge) const noexcept;

		void renderRoadHighlights(const std::unordered_set<size_t>& edgeIds,
			const std::unordered_map<size_t, glm::vec2>& ownedRoadPosByEdge,
			const std::unordered_map<size_t, int>& ownedRoadEdgeIndexByEdge,
			float time,
			float baseAlpha,
			const glm::mat4& view,
			const glm::mat4& projection) noexcept;

		void updateDustSpawns(float time) noexcept;

		void renderSettlements(const glm::mat4& view, const glm::mat4& projection, float time) noexcept;

		void updateSettlementHover(const glm::mat4& view, const glm::mat4& projection, const Camera& cam, float time) noexcept;

		void renderRoads(const glm::mat4& view,
			const glm::mat4& projection,
			std::unordered_map<size_t, Entity>& ownedRoadEntitiesByEdge,
			std::unordered_map<size_t, int>& ownedRoadEdgeIndexByEdge,
			std::unordered_map<size_t, glm::vec2>& ownedRoadPosByEdge) noexcept;
			
		void updateRoadHover(const glm::mat4& view,
			const glm::mat4& projection,
			const Camera& cam,
			float time,
			const std::unordered_map<size_t, Entity>& ownedRoadEntitiesByEdge,
			const std::unordered_map<size_t, int>& ownedRoadEdgeIndexByEdge,
			const std::unordered_map<size_t, glm::vec2>& ownedRoadPosByEdge) noexcept;

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
		std::array<Texture, 5> woodSettlementTextures;  // wood settlement textures for animation
		std::array<Texture, 6> stoneSettlementTextures; // stone settlement textures for animation

		GLuint m_quad_vao;
		GLuint m_quad_ebo;

		Viewport viewport;

		std::mt19937 rng;
		std::vector<DustPuff> dustPuffs;
		size_t lastSettlementCount = 0;
		size_t lastRoadCount = 0;

		std::unordered_set<size_t> lastHoveredConnectedEdges;
		size_t lastHoveredEdgeId = SIZE_MAX;
		float hoverFadeStartTime = 0.0f;
		float hoverFadeDuration = 0.25f;
		bool hoverActive = false;

		Entity lastHoveredSettlementEntity = Entity();
		glm::vec2 lastHoveredSettlementPos = glm::vec2(0.0f);
		glm::vec2 lastHoveredSettlementScale = glm::vec2(1.0f);
		float settlementHoverFadeStartTime = 0.0f;
		float settlementHoverFadeDuration = 0.2f;
		bool settlementHoverActive = false;
	};
} // namespace df
