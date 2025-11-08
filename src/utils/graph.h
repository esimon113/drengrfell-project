#pragma once

#include <common.h>
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <array>
#include <concepts>



namespace df {

    // structs are just placeholders for now:
    struct Tile {
        size_t id;
        bool operator==(const Tile& other) const { return id == other.id; }
    };

    struct Edge {
        size_t id;

        Edge() : id(SIZE_MAX){} // init id
        bool isBuildable(size_t playerId) { return false; }
        bool operator==(const Edge& other) const { return id == other.id; }
    };

    struct Vertex {
        size_t id;

        Vertex() : id(SIZE_MAX) {}
        bool isBuildable(size_t playerId) { return false; }
        bool operator==(const Vertex& other) const { return id == other.id; }
    };


    // Go counter-clockwise from north:
    enum class TileDirection {
        NORTH = 0,
        NORTH_WEST,
        SOUTH_EAST,
        SOUTH,
        SOUTH_WEST,
        NORTH_EAST
    };

    inline std::tuple<size_t, size_t> getTileDirectionCoordinates(TileDirection direction) {
        switch (direction) {
            case TileDirection::NORTH:       return { 0, 1 };
            case TileDirection::SOUTH:       return { 0, -1 };
            case TileDirection::NORTH_EAST:  return { 1, 1 };
            case TileDirection::NORTH_WEST:  return { -1, 1 };
            case TileDirection::SOUTH_EAST:  return { 1, -1 };
            case TileDirection::SOUTH_WEST:  return { -1, -1 };
            default: return { 0, 0 };
        }
    }



    // make sure T has id
    template<typename T>
    concept HasIdProperty = requires (T t) {
    	{ t.id } -> std::convertible_to<size_t>;
    };




    class Graph {
        public:
            Graph() = default;
            ~Graph() = default;

            void addTile(const Tile& tile);
            void addEdge(const Edge& edge);
            void addVertex(const Vertex& vertex);

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

            const std::vector<Tile>& getTiles() const { return tiles; }
            const std::vector<Edge>& getEdges() const { return edges; }
            const std::vector<Vertex>& getVertices() const { return vertices; }


            // Allow for storing and loading:
            std::string serialize() const;
            void deserialize(const std::string& data) const;


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


            // Algorithms that might come in handy;
            template<HasIdProperty T>
            std::vector<T> breadthFirstSearch(const T& start) const;
            template<HasIdProperty T>
            std::vector<T> depthFirstSearch(const T& start) const;
            template<HasIdProperty T>
            std::vector<T> dijkstra(const T& start) const;

            std::vector<size_t> getNeighborIds(size_t id) const;

    };
}
