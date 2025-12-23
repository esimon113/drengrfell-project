#pragma once

#include <common.h>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <vector>
#include <unordered_map>
#include <array>
#include <concepts>


#include <nlohmann/json.hpp>

#include "worldGeneratorConfig.h"
using json = nlohmann::json;


#include "tile.h"
#include "edge.h"
#include "vertex.h"


namespace df {


	// make sure T has id
	template<typename T>
	concept HasIdProperty = requires (T t) {
		{ t.getId() } -> std::convertible_to<size_t>;
	};


	class Graph {
		public:
			Graph() = default;
			~Graph() = default;

			void addTile(std::unique_ptr<Tile> tile);
			void addEdge(std::unique_ptr<Edge> edge);
			void addVertex(std::unique_ptr<Vertex> vertex);

			TileHandle getTile(size_t index);
			EdgeHandle getEdge(size_t index);
			VertexHandle getVertex(size_t index);

			void removeTile(const TileHandle tile);
			void removeEdge(const EdgeHandle edge);
			void removeVertex(const VertexHandle vertex);

			void connectEdgeToTile(const TileHandle tile, const EdgeHandle edge);
			void connectVertexToEdge(const EdgeHandle edge, const VertexHandle vertex);
			void connectVertexToTile(const VertexHandle vertex, const TileHandle tile);

			std::optional<std::array<EdgeHandle, 6>> getTileEdges(const TileHandle tile);
			std::optional<std::array<VertexHandle, 6>> getTileVertices(const TileHandle tile);
			std::optional<std::array<VertexHandle, 2>> getEdgeVertices(const EdgeHandle edge);
			std::optional<std::array<EdgeHandle, 3>> getVertexEdges(const VertexHandle vertex);
			std::optional<std::array<TileHandle, 3>> getVertexTiles(const VertexHandle vertex);

			size_t getEdgeIndex(size_t edgeId);

			const std::vector<std::unique_ptr<Tile>>& getTiles() const { return this->tiles; }
			const std::vector<std::unique_ptr<Edge>>& getEdges() const { return this->edges; }
			const std::vector<std::unique_ptr<Vertex>>& getVertices() const { return this->vertices; }

			size_t getTileCount() const { return this->tiles.size(); }
			size_t getEdgeCount() const { return this->edges.size(); }
			size_t getVertexCount() const { return this->vertices.size(); }

			// Allow for storing and loading:
			json serialize() const;
			void deserialize(const std::string& data);

			// no game state included, only map topology...
			void save(std::filesystem::path& to);
			void load(std::filesystem::path& from);


			// Algorithms that might come in handy;
			template<HasIdProperty T>
			std::vector<T> breadthFirstSearch(const T& start) const;
			template<HasIdProperty T>
			std::vector<T> depthFirstSearch(const T& start) const;
			template<HasIdProperty T>
			std::vector<T> dijkstra(const T& start) const;

			template<HasIdProperty T>
			size_t getDistanceBetween(const T& start, const T& end) const;

			// Methods for using the graph as a rectangular map
			void regenerate(const WorldGeneratorConfig &worldGeneratorConfig = WorldGeneratorConfig());
			unsigned getMapWidth() const { return this->mapWidth; }
			void setMapWidth(const unsigned width) { this->mapWidth = width; }
			bool isRenderUpdateRequested() const { return this->renderUpdateRequested; }
			void setRenderUpdateRequested(const bool value) { this->renderUpdateRequested = value; }


		private:
			// nodes OWNED by the graph
			std::vector<std::unique_ptr<Tile>> tiles;
			std::vector<std::unique_ptr<Edge>> edges;
			std::vector<std::unique_ptr<Vertex>> vertices;

			// size_t = ids
			std::unordered_map<size_t, std::array<EdgeHandle, 6>> tileEdges;
			std::unordered_map<size_t, std::array<VertexHandle, 6>> tileVertices;

			// would it be worth it to add edgeTiles?!
			std::unordered_map<size_t, std::array<VertexHandle, 2>> edgeVertices;

			std::unordered_map<size_t, std::array<EdgeHandle, 3>> vertexEdges;
			std::unordered_map<size_t, std::array<TileHandle, 3>> vertexTiles;

			bool doesTileExist(const TileHandle tile);
			bool doesTileExist(size_t tileId);
			bool doesEdgeExist(const EdgeHandle edge);
			bool doesEdgeExist(size_t edgeId);
			bool doesVertexExist(const VertexHandle vertex);
			bool doesVertexExist(size_t vertexId);

			// write tiles from vector into graph
			void initializeTilesForGraph(std::vector<Tile> newTiles);
			void populate();

			std::vector<size_t> getNeighborIds(size_t id) const;

			// Methods for using the graph as a rectangular map
			unsigned mapWidth = 0;
			bool renderUpdateRequested = false;
	};
}
