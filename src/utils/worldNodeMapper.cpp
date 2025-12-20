#include "worldNodeMapper.h"
#include "fmt/base.h"

#include <cstdint>
#include <array>
#include <limits>
#include <unordered_set>



namespace df {
	// For now: simple distance calculation based on tile layout.
	// x-pos = "currentROw & 1" -> odd row? -> odd rows shift right by half tile, even by 0
	// y-pos = hex rows overlap vertcially, so vertical distance between rows is 3/4 of hexagon heigth
	glm::vec2 WorldNodeMapper::getTilePosition(uint32_t row, uint32_t col) noexcept {
		return glm::vec2(2.0f * (col + 0.5f * (row & 1)), row * 1.5f);
	}


	// calculate positions of 6 vertices for the tile (relative to the tile center)
	std::array<glm::vec2, 6> WorldNodeMapper::getVertexOffsets(const float hexagonRadius) noexcept {
		const float sqrt3 = 1.732050808f;

		return {
			glm::vec2(0.0f, hexagonRadius),									// top
			glm::vec2(0.5f * sqrt3 * hexagonRadius, 0.5f * hexagonRadius),	// top-right
			glm::vec2(0.5f * sqrt3 * hexagonRadius, -0.5f * hexagonRadius),	// bottom-right
			glm::vec2(0.0f, -hexagonRadius),								// bottom
			glm::vec2(-0.5f * sqrt3 * hexagonRadius, -0.5f * hexagonRadius),// bottom-left
			glm::vec2(-0.5f * sqrt3 * hexagonRadius, 0.5f * hexagonRadius)	// top-left
		};
	}


	std::optional<size_t> WorldNodeMapper::findClosestTileToWorldPos(const glm::vec2 &worldPos, const Graph& map) noexcept {
		if (map.getTileCount() == 0) return std::nullopt;

		float minDistance = std::numeric_limits<float>::max();
		size_t closestTileId = SIZE_MAX;
		uint32_t columns = map.getMapWidth();

		for (size_t tileId = 0; tileId < map.getTileCount(); ++tileId) {
			// calculate tile position based on tileId
			uint32_t currentRow = tileId / columns;
			uint32_t currentCol = tileId % columns;

			glm::vec2 tileCenterPos(WorldNodeMapper::getTilePosition(currentRow, currentCol));

			float distance = glm::distance(worldPos, tileCenterPos);
			if (distance < minDistance) {
				minDistance = distance;
				closestTileId = tileId;
			}
		}

		if (closestTileId != SIZE_MAX) return closestTileId;
		return std::nullopt;
	};


	std::optional<size_t> WorldNodeMapper::findClosestVertexToWorldPos(const glm::vec2 &worldPos, const Graph &map) noexcept {
		fmt::println("[WorldNodeMapper] findClosestVertexToWorldPos: searching for vertex near world position ({}, {})", worldPos.x, worldPos.y);

		if (map.getVertexCount() == 0) {
			fmt::println("[WorldNodeMapper] No vertices in map, returning nullopt");
			return std::nullopt;
		}

		const float hexagonRadius = 1.0f;
		const uint32_t columns = map.getMapWidth();
		float minDistance = std::numeric_limits<float>::max();
		size_t closestVertexId = SIZE_MAX;
		std::unordered_set<size_t> processedVertexIds;

		fmt::println("[WorldNodeMapper] Map has {} tiles, {} columns", map.getTileCount(), columns);

		for (size_t tileId = 0; tileId < map.getTileCount(); ++tileId) {
			const Tile& tile = map.getTile(tileId);

			uint32_t currentRow = tileId / columns;
			uint32_t currentCol = tileId % columns;

			glm::vec2 tileCenterPos(WorldNodeMapper::getTilePosition(currentRow, currentCol));
			const auto vertices = map.getTileVertices(tile);
			std::array<glm::vec2, 6> vertexOffsets = WorldNodeMapper::getVertexOffsets(hexagonRadius);

			for (size_t i = 0; i < vertices.size(); ++i) {
				const Vertex& vertex = vertices[i];
				size_t vertexId = vertex.getId();

				if (vertexId == SIZE_MAX) continue;

				// Skip if already processed the vertex (-> shared between tiles)
				if (processedVertexIds.find(vertexId) != processedVertexIds.end()) continue;
				processedVertexIds.insert(vertexId);

				glm::vec2 vertexPosition = tileCenterPos + vertexOffsets[i];
				float distance = glm::distance(worldPos, vertexPosition);

				if (distance < minDistance) {
					fmt::println("[WorldNodeMapper] New closest vertex found: vertexId={}, distance={:.3f}, position=({:.3f}, {:.3f})",
						vertexId, distance, vertexPosition.x, vertexPosition.y);
					minDistance = distance;
					closestVertexId = vertexId;
				}
			}
		}

		if (closestVertexId != SIZE_MAX) {
			fmt::println("[WorldNodeMapper] Closest vertex: vertexId={}, distance={:.3f}", closestVertexId, minDistance);
			return closestVertexId;
		}

		fmt::println("[WorldNodeMapper] No valid vertex found, returning nullopt");
		return std::nullopt;
	};


	std::optional<size_t> WorldNodeMapper::findClosestEdgeToWorldPos(const glm::vec2 &worldPos, const Graph &map) noexcept {
		fmt::println("[WorldNodeMapper] findClosestEdgeToWorldPos: searching for edge near world position ({}, {})", worldPos.x, worldPos.y);

		if (map.getEdgeCount() == 0) {
			fmt::println("[WorldNodeMapper] No edges in map, returning nullopt");
			return std::nullopt;
		}

		const float hexagonRadius = 1.0f;
		const uint32_t columns = map.getMapWidth();
		float minDistance = std::numeric_limits<float>::max();
		size_t closestEdgeId = SIZE_MAX;
		std::unordered_set<size_t> processedEdgeIds;

		fmt::println("[WorldNodeMapper] Map has {} tiles, {} columns", map.getTileCount(), columns);

		for (size_t tileId = 0; tileId < map.getTileCount(); ++tileId) {
			const Tile& tile = map.getTile(tileId);

			uint32_t currentRow = tileId / columns;
			uint32_t currentCol = tileId % columns;

			glm::vec2 tileCenterPos(WorldNodeMapper::getTilePosition(currentRow, currentCol));
			const auto edges = map.getTileEdges(tile);
			std::array<glm::vec2, 6> vertexOffsets = WorldNodeMapper::getVertexOffsets(hexagonRadius);

			for (size_t i = 0; i < edges.size(); ++i) {
				const Edge& edge = edges[i];
				size_t edgeId = edge.getId();

				if (edgeId == SIZE_MAX) continue;

				// Skip if already processed edge (-> shared between tiles)
				if (processedEdgeIds.find(edgeId) != processedEdgeIds.end()) continue;
				processedEdgeIds.insert(edgeId);

				// edge-position as center between two neighboring vertices
				glm::vec2 vertex1Position = tileCenterPos + vertexOffsets[i];
				glm::vec2 vertex2Position = tileCenterPos + vertexOffsets[(i + 1) % 6];
				glm::vec2 edgePosition = (vertex1Position + vertex2Position) / 2.0f;

				float distance = glm::distance(worldPos, edgePosition);
				if (distance < minDistance) {
					fmt::println("[WorldNodeMapper] New closest edge found: edgeId={}, distance={:.3f}, position=({:.3f}, {:.3f})",
						edgeId, distance, edgePosition.x, edgePosition.y);
					minDistance = distance;
					closestEdgeId = edgeId;
				}
			}
		}

		if (closestEdgeId != SIZE_MAX) {
			fmt::println("[WorldNodeMapper] Closest edge: edgeId={}, distance={:.3f}", closestEdgeId, minDistance);
			return closestEdgeId;
		}

		fmt::println("[WorldNodeMapper] No valid edge found, returning nullopt");
		return std::nullopt;
	};


	glm::vec2 WorldNodeMapper::getWorldPositionForVertex(size_t vertexId, const Graph& map) noexcept {
		const float hexagonRadius = 1.0f;
		const uint32_t columns = map.getMapWidth();

		// find tiles that have this vertex
		for (size_t tileId = 0; tileId < map.getTileCount(); ++tileId) {
			const Tile& tile = map.getTile(tileId);
			const auto vertices = map.getTileVertices(tile);

			for (size_t i = 0; i < vertices.size(); ++i) {
				if (vertices[i].getId() == vertexId) {
					uint32_t row = tileId / columns;
					uint32_t col = tileId % columns;

					glm::vec2 tileCenterPos = WorldNodeMapper::getTilePosition(row, col);
					std::array<glm::vec2, 6> vertexOffsets = WorldNodeMapper::getVertexOffsets(hexagonRadius);

					return tileCenterPos + vertexOffsets[i];
				}
			}
		}
		return glm::vec2(0.0f);
	}


	glm::vec2 WorldNodeMapper::getWorldPositionForEdge(size_t edgeId, const Graph& map) noexcept {
		const float hexagonRadius = 1.0f;
		const uint32_t columns = map.getMapWidth();

		// Find tiles that have this edge
		for (size_t tileId = 0; tileId < map.getTileCount(); ++tileId) {
			const Tile& tile = map.getTile(tileId);
			const auto edges = map.getTileEdges(tile);

			for (size_t i = 0; i < edges.size(); ++i) {
				if (edges[i].getId() == edgeId) {
					uint32_t row = tileId / columns;
					uint32_t col = tileId % columns;

					glm::vec2 tileCenterPos = WorldNodeMapper::getTilePosition(row, col);
					std::array<glm::vec2, 6> vertexOffsets = WorldNodeMapper::getVertexOffsets(hexagonRadius);

					// Edge position is middle between two vertices
					glm::vec2 vertex1Position = tileCenterPos + vertexOffsets[i];
					glm::vec2 vertex2Position = tileCenterPos + vertexOffsets[(i + 1) % 6];

					return (vertex1Position + vertex2Position) / 2.0f;
				}
			}
		}
		return glm::vec2(0.0f);
	}
}
