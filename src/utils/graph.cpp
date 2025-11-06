#include <stdexcept>
#include <string>
#include <unordered_set>
#include <queue>
#include <stack>

#include "graph.h"




namespace df {
    void Graph::addTile(const Tile& tile) {
        if (std::find(this->tiles.begin(), this->tiles.end(), tile) == this->tiles.end()) {
            this->tiles.push_back(tile);

            std::array<Edge, 6> edges;
            std::array<Vertex, 6> vertices;

            // added with default values (from constructors)
            this->tileEdges[tile.id] = edges;
            this->tileVertices[tile.id] = vertices;
        }
    }


    void Graph::addEdge(const Edge& edge) {
        if (std::find(this->edges.begin(), this->edges.end(), edge) == this->edges.end()) {
            this->edges.push_back(edge);

            std::array<Vertex, 2> vertices;

            this->edgeVertices[edge.id] = vertices;
        }
    }


    void Graph::addVertex(const Vertex& vertex) {
        if (std::find(this->vertices.begin(), this->vertices.end(), vertex) == this->vertices.end()) {
            this->vertices.push_back(vertex);

            std::array<Edge, 3> edges;
            std::array<Tile, 3> tiles;

            this->vertexEdges[vertex.id] = edges;
            this->vertexTiles[vertex.id] = tiles;
        }
    }


    void Graph::removeTile(const Tile& tile) {
        if (auto it = std::find(this->tiles.begin(), this->tiles.end(), tile); it != this->tiles.end()) {
            this->tiles.erase(it);
            this->tileEdges.erase(it->id);
            this->tileVertices.erase(it->id);
        }
    }


    void Graph::removeEdge(const Edge& edge) {
        if (auto it = std::find(this->edges.begin(), this->edges.end(), edge); it != this->edges.end()) {
            this->edges.erase(it);
            this->edgeVertices.erase(it->id);
        }        
    }


    void Graph::removeVertex(const Vertex& vertex) {
        if (auto it = std::find(this->vertices.begin(), this->vertices.end(), vertex); it != this->vertices.end()) {
            this->vertices.erase(it);
            this->vertexEdges.erase(it->id);
            this->vertexTiles.erase(it->id);
        }        
    }


    void Graph::connectEdgeToTile(const Tile& tile, const Edge& edge) {
        throw std::runtime_error(std::string("NOT IMPLEMENTED: ") + __PRETTY_FUNCTION__);
    }


    void Graph::connectVertexToEdge(const Edge& edge, const Vertex& vertex) {
        throw std::runtime_error(std::string("NOT IMPLEMENTED: ") + __PRETTY_FUNCTION__);
    }


    void Graph::connectVertexToTile(const Vertex& vertex, const Tile& tile) {
        throw std::runtime_error(std::string("NOT IMPLEMENTED: ") + __PRETTY_FUNCTION__);
    }


    /**
     * Returns an empty array {} if the tile is not found.
     */
    std::array<Edge, 6> Graph::getTileEdges(const Tile& tile) const {
        if (auto it = this->tileEdges.find(tile.id); it != this->tileEdges.end()) {
            return it->second;
        }
        return {};
    }


    /**
     * Returns an empty array {} if the tile is not found.
     */
    std::array<Vertex, 6> Graph::getTileVertices(const Tile& tile) const {
        if (auto it = this->tileVertices.find(tile.id); it != this->tileVertices.end()) {
            return it->second;
        }
        return {};
    }


    /**
     * Returns an empty array {} if the edge is not found.
     */
    std::array<Vertex, 2> Graph::getEdgeVertices(const Edge& edge) const {
        if (auto it = this->edgeVertices.find(edge.id); it != this->edgeVertices.end()) {
            return it->second;
        }
        return {};
    }


    /**
     * Returns an empty array {} if the vertex is not found.
     */
    std::array<Edge, 3> Graph::getVertexEdges(const Vertex& vertex) const {
        if (auto it = this->vertexEdges.find(vertex.id); it != this->vertexEdges.end()) {
            return it->second;
        }
        return {};
    }


    /**
     * Returns an empty array {} if the vertex is not found.
     */
    std::array<Tile, 3> Graph::getVertexTiles(const Vertex& vertex) const {
        if (auto it = this->vertexTiles.find(vertex.id); it != this->vertexTiles.end()) {
            return it->second;
        }
        return {};
    }


    std::string Graph::serialize() const {
        throw std::runtime_error(std::string("NOT IMPLEMENTED: ") + __PRETTY_FUNCTION__);
    }


    void Graph::deserialize(const std::string& data) const {
        throw std::runtime_error(std::string("NOT IMPLEMENTED: ") + __PRETTY_FUNCTION__);
    }





    /**
     * Gets ids of *all* neighbors, regardless of node-type. 
     * This means that ids must be unique over all node-types
     */
    std::vector<size_t> Graph::getNeighborIds(size_t id) const {
        std::vector<size_t> neighbors;

        // Check for id being of a Tile:
        if (auto it = this->tileEdges.find(id); it != this->tileEdges.end()) {
            for (const auto& edge : it->second) {
                if (edge.id != SIZE_MAX) {
                    neighbors.push_back(edge.id);
                }
            }
        }
        if (auto it = this->tileVertices.find(id); it != this->tileVertices.end()) {
            for (const auto& vertex : it->second) {
                if (vertex.id != SIZE_MAX) {
                    neighbors.push_back(vertex.id);
                }
            }
        }

        // Check for id being of an edge:
        if (auto it = this->edgeVertices.find(id); it != this->edgeVertices.end()) {
            for (const auto& vertex : it->second) {
                if (vertex.id != SIZE_MAX) {
                    neighbors.push_back(vertex.id);
                }
            }
        }

        // Check for id being of a vertex:
        if (auto it = this->vertexEdges.find(id); it != this->vertexEdges.end()) {
            for (const auto& edge : it->second) {
                if (edge.id != SIZE_MAX) {
                    neighbors.push_back(edge.id);
                }
            }
        }
        if (auto it = this->vertexTiles.find(id); it != this->vertexTiles.end()) {
            for (const auto& tile : it->second) {
                if (tile.id != SIZE_MAX) {
                    neighbors.push_back(tile.id);
                }
            }
        }

        return neighbors;
    }


    // TODO: make sure that T has attribute "id"
    /**
     * BFS finds shortest number of hops between nodes. 
     * This can be used for checking road distances etc. 
     * Search the graph into the breadth first -> first all nodes with distance 1, 
     * then all with distance 2, etc.
     */
    template <typename T>
    std::vector<T> Graph::breadthFirstSearch(const T& start) const {
        std::vector<T> sequence;
        std::unordered_set<size_t> visitedIds;
        std::queue<size_t> q;

        q.push(start.id);
        visitedIds.insert(id);

        while (!q.empty()) {
            size_t currentId = q.front();
            q.pop();
            sequence.push_back(T{currentId});

            for (size_t neighbor : this->getNeighborIds(currentId)) {
                if (!visitedIds.count(neighbor)) {
                    visitedIds.insert(neighbor);
                    q.push(neighbor);
                }
            }
        }

        return sequence;
    }


    // same as with BFS: make sure that T has "id"
    template <typename T>
    std::vector<T> Graph::depthFirstSearch(const T& start) const {
        std::vector<T> sequence;
        std::unordered_set<size_t> visited;
        std::stack<size_t> s;

        s.push(start.id);

        while (!s.empty()) {
            size_t currentId = s.top();
            s.pop();

            if (visited.count(currentId)) {
                continue;
            }

            visited.insert(currentId);
            sequence.push_back(T{currentId});

            auto neighbors = this->getNeighborIds(currentId);
            std::reverse(neighbors.begin(), neighbors.end());

            for (size_t neighbor : neighbors) {
                if (!visited.count(neighbor)) {
                    s.push(neighbor);
                }
            }
        }
    
        return sequence;
    }


    
}
