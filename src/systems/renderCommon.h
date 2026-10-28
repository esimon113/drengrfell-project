#pragma once
#include "gamestate.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "resultError.h"

/*
 * This header provides common functionality for render systems.
 */


namespace df {

	struct Viewport {
		glm::uvec2 origin;
		glm::uvec2 size;
	};

	glm::vec2 screenToWorldCoordinates(const glm::vec2& screenPos, Viewport viewport, glm::vec2 worldDimensions) noexcept;
	glm::vec2 calculateWorldDimensions(int columns, int rows) noexcept;

	// Other owners get a light tint. The local viewer's pieces stay uncolored.
	// This is used to enable a player to visually differentiate between his own and other player's buildings/hero
	inline glm::vec3 ownershipTint(size_t ownerId, size_t viewerPlayerId) noexcept {
		if (ownerId == viewerPlayerId) {
			return glm::vec3(1.0f);
		}
		return glm::vec3(0.45f, 0.75f, 1.0f);
	}

	namespace RenderCommon {

		template <typename ReturnType>
		ReturnType getMapColumns(const Graph& map) noexcept {
			return static_cast<ReturnType>(map.getMapWidth());
		}

		template <typename ReturnType>
		ReturnType getMapRows(const Graph& map) noexcept {
			return static_cast<ReturnType>(map.getTileCount() / getMapColumns<ReturnType>(map));
		}

		inline glm::vec2 rowColToWorldCoordinates(const int column, const int row) noexcept {
			return glm::vec2{
				2.0f * (static_cast<float>(column) + 0.5f * static_cast<float>(row & 1)),
				static_cast<float>(row) * 1.5};
		}

		inline glm::ivec2 worldToRowColCoordinates(const glm::vec2& position) noexcept {
			const int row = static_cast<int>(std::lround(position.y / 1.5f));
			const int col = static_cast<int>(std::lround(position.x * 0.5f - 0.5f * static_cast<float>(row & 1)));
			return {col, row};
		}

	} // namespace RenderCommon
} // namespace df
