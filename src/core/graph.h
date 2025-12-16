#pragma once

#include <common.h>
#include <cstddef>
#include <filesystem>
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

            void addTile(const Tile& tile);
            void addEdge(const Edge& edge);
            void addVertex(const Vertex& vertex);

            Tile& getTile(size_t index);
            const Tile& getTile(size_t index) const;
            Edge& getEdge(size_t index);
            const Edge& getEdge(size_t index) const;
            Vertex& getVertex(size_t index);
            const Vertex& getVertex(size_t index) const;

            void removeTile(const Tile& tile);
            void removeEdge(const Edge& edge);
            void removeVertex(const Vertex& vertex);

            void connectEdgeToTile(const Tile& tile, const Edge& edge);
            void connectVertexToEdge(const Edge& edge, const Vertex& vertex);
            void connectVertexToTile(const Vertex& vertex, const Tile& tile);

            std::array<Edge, 6> getTileEdges(const Tile& tile) const;
            std::array<Vertex, 6> getTileVertices(const Tile& tile) const;
            std::array<Vertex, 2> getEdgeVertices(const Edge& edge) const;
            std::array<Edge, 3> getVertexEdges(const Vertex& vertex) const;
            std::array<Tile, 3> getVertexTiles(const Vertex& vertex) const;

            const std::vector<Tile>& getTiles() const { return this->tiles; }
            const std::vector<Edge>& getEdges() const { return this->edges; }
            const std::vector<Vertex>& getVertices() const { return this->vertices; }

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
            std::vector<Tile> tiles;
            std::vector<Edge> edges;
            std::vector<Vertex> vertices;

            // size_t = ids
            std::unordered_map<size_t, std::array<Edge, 6>> tileEdges;
            std::unordered_map<size_t, std::array<Vertex, 6>> tileVertices;

            std::unordered_map<size_t, std::array<Vertex, 2>> edgeVertices;

            std::unordered_map<size_t, std::array<Edge, 3>> vertexEdges;
            std::unordered_map<size_t, std::array<Tile, 3>> vertexTiles;

            bool doesTileExist(const Tile& tile);
            bool doesEdgeExist(const Edge& edge);
            bool doesVertexExist(const Vertex& vertex);

            std::vector<size_t> getNeighborIds(size_t id) const;

            // Methods for using the graph as a rectangular map
            unsigned mapWidth = 0;
            bool renderUpdateRequested = false;
    };
}
