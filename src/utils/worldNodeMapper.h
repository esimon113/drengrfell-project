#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <optional>

#include "graph.h"


namespace df {
	class WorldNodeMapper {
		public:
			static std::optional<size_t> findClosestTileToWorldPos(const glm::vec2& worldPos, const Graph& map) noexcept;
			static std::optional<size_t> findClosestVertexToWorldPos(const glm::vec2& worldPos, const Graph& map) noexcept;
			static std::optional<size_t> findClosestEdgeToWorldPos(const glm::vec2& worldPos, const Graph& map) noexcept;
			
			// Helper methods for calculating positions
			static glm::vec2 getTilePosition(uint32_t row, uint32_t col) noexcept;
			static std::array<glm::vec2, 6> getVertexOffsets(const float hexagonRadius) noexcept;
	};
}
