#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <algorithm>
#include <queue>
#include <stack>
#include <unordered_set>
#include <fstream>

#include "graph.h"

#include "worldGenerator.h"


namespace df {
    void Graph::addTile(const Tile& tile) {
        if (std::find(this->tiles.begin(), this->tiles.end(), tile) == this->tiles.end()) {
            this->tiles.push_back(tile);

            std::array<Edge, 6> edgesToAdd{};
            std::array<Vertex, 6> verticesToAdd{};

            // added with default values (from constructors)
            this->tileEdges[tile.getId()] = edgesToAdd;
            this->tileVertices[tile.getId()] = verticesToAdd;
        }
    }


    void Graph::addEdge(const Edge& edge) {
        if (std::find(this->edges.begin(), this->edges.end(), edge) == this->edges.end()) {
            this->edges.push_back(edge);

            std::array<Vertex, 2> verticesToAdd{};

            this->edgeVertices[edge.getId()] = verticesToAdd;
        }
    }


    void Graph::addVertex(const Vertex& vertex) {
        if (std::find(this->vertices.begin(), this->vertices.end(), vertex) == this->vertices.end()) {
            this->vertices.push_back(vertex);

            std::array<Edge, 3> edgesToAdd{};
            std::array<Tile, 3> tilesToAdd{};

            this->vertexEdges[vertex.getId()] = edgesToAdd;
            this->vertexTiles[vertex.getId()] = tilesToAdd;
        }
    }


    Tile& Graph::getTile(size_t index) {
    	if (index >= this->tiles.size()) {
    		throw std::out_of_range("Tile index out of range");
    	}

    	return this->tiles[index];
    }

    const Tile& Graph::getTile(size_t index) const {
    	if (index >= this->tiles.size()) {
    		throw std::out_of_range("Tile index out of range");
    	}

    	return this->tiles[index];
    }


    Edge& Graph::getEdge(size_t index) {
    	if (index >= this->edges.size()) {
    		throw std::out_of_range("Edge index out of range");
    	}

    	return this->edges[index];
    }

    const Edge& Graph::getEdge(size_t index) const {
    	if (index >= this->edges.size()) {
    		throw std::out_of_range("Edge index out of range");
    	}

    	return this->edges[index];
    }


    Vertex& Graph::getVertex(size_t index) {
    	if (index >= this->vertices.size()) {
     		throw std::out_of_range("Vertex index out of range");
     	}

    	return this->vertices[index];
    }

    const Vertex& Graph::getVertex(size_t index) const {
    	if (index >= this->vertices.size()) {
     		throw std::out_of_range("Vertex index out of range");
     	}

    	return this->vertices[index];
    }


    bool Graph::doesTileExist(const Tile& tile) {
        return std::find(this->tiles.begin(), this->tiles.end(), tile) != this->tiles.end();
    }


    bool Graph::doesEdgeExist(const Edge& edge) {
        return std::find(this->edges.begin(), this->edges.end(), edge) != this->edges.end();
    }


    bool Graph::doesVertexExist(const Vertex& vertex) {
        return std::find(this->vertices.begin(), this->vertices.end(), vertex) != this->vertices.end();
    }


    void Graph::removeTile(const Tile& tile) {
        if (auto it = std::find(this->tiles.begin(), this->tiles.end(), tile); it != this->tiles.end()) {
            this->tiles.erase(it);
            this->tileEdges.erase(it->getId());
            this->tileVertices.erase(it->getId());
        }
    }


    void Graph::removeEdge(const Edge& edge) {
        if (auto it = std::find(this->edges.begin(), this->edges.end(), edge); it != this->edges.end()) {
            this->edges.erase(it);
            this->edgeVertices.erase(it->getId());
        }
    }


    void Graph::removeVertex(const Vertex& vertex) {
        if (auto it = std::find(this->vertices.begin(), this->vertices.end(), vertex); it != this->vertices.end()) {
            this->vertices.erase(it);
            this->vertexEdges.erase(it->getId());
            this->vertexTiles.erase(it->getId());
        }
    }


    void Graph::connectEdgeToTile(const Tile& tile, const Edge& edge) {
    	if (!this->doesTileExist(tile)) { throw std::invalid_argument("Tile not found"); }
    	if (!this->doesEdgeExist(edge)) { throw std::invalid_argument("Edge not found"); }

  		auto& localEdges = this->tileEdges[tile.getId()];
    	for (size_t i = 0; i < 6; ++i) {
     		if (localEdges[i].getId() == SIZE_MAX) {
    			localEdges[i] = edge;
    			break;
       		}
     	}
    }


    void Graph::connectVertexToEdge(const Edge& edge, const Vertex& vertex) {
    	if (!this->doesEdgeExist(edge)) { throw std::invalid_argument("Edge not found"); }
    	if (!this->doesVertexExist(vertex)) { throw std::invalid_argument("Vertex not found"); }

  		auto& localVertices = this->edgeVertices[edge.getId()];
    	for (size_t i = 0; i < 2; ++i) {
     		if (localVertices[i].getId() == SIZE_MAX) {
    			localVertices[i] = vertex;
       			break;
       		}
     	}
    }


    void Graph::connectVertexToTile(const Vertex& vertex, const Tile& tile) {
    	if (!this->doesVertexExist(vertex)) { throw std::invalid_argument("Vertex not found"); }
    	if (!this->doesTileExist(tile)) { throw std::invalid_argument("Tile not found"); }

  		auto& localVertices = this->tileVertices[tile.getId()];
        for (size_t i = 0; i < 6; ++i) {
      		if (localVertices[i].getId() == SIZE_MAX) {
     			localVertices[i] = vertex;
        		break;
        	}
        }
    }


    /**
     * Returns an empty array {} if the tile is not found.
     */
    std::array<Edge, 6> Graph::getTileEdges(const Tile& tile) const {
        if (auto it = this->tileEdges.find(tile.getId()); it != this->tileEdges.end()) {
            return it->second;
        }
        return {};
    }


    /**
     * Returns an empty array {} if the tile is not found.
     */
    std::array<Vertex, 6> Graph::getTileVertices(const Tile& tile) const {
        if (auto it = this->tileVertices.find(tile.getId()); it != this->tileVertices.end()) {
            return it->second;
        }
        return {};
    }


    /**
     * Returns an empty array {} if the edge is not found.
     */
    std::array<Vertex, 2> Graph::getEdgeVertices(const Edge& edge) const {
        if (auto it = this->edgeVertices.find(edge.getId()); it != this->edgeVertices.end()) {
            return it->second;
        }
        return {};
    }


    /**
     * Returns an empty array {} if the vertex is not found.
     */
    std::array<Edge, 3> Graph::getVertexEdges(const Vertex& vertex) const {
        if (auto it = this->vertexEdges.find(vertex.getId()); it != this->vertexEdges.end()) {
            return it->second;
        }
        return {};
    }


    /**
     * Returns an empty array {} if the vertex is not found.
     */
    std::array<Tile, 3> Graph::getVertexTiles(const Vertex& vertex) const {
        if (auto it = this->vertexTiles.find(vertex.getId()); it != this->vertexTiles.end()) {
            return it->second;
        }
        return {};
    }


    json Graph::serialize() const {
    	json j;

     	for (const auto& tile : this->tiles) {
      		json tileJson;
        	tileJson["id"] = tile.getId();
			tileJson["meta"] = tile.serialize();

			json edgesJson;
			for (const auto& edge : this->tileEdges.at(tile.getId())) {
				const auto& v = this->edgeVertices.at(edge.getId());
				edgesJson[std::to_string(edge.getId())] = { v.at(0).getId(), v.at(1).getId() };
			}

			tileJson["edges"] = edgesJson;
			j[std::to_string(tile.getId())] = tileJson;
      	}

      	return j;
    }


    // TODO: add error handling + sanity checks
    // -> currently assumes correct JSON structure
    void Graph::deserialize(const std::string& data) {
    	json j = json::parse(data);

		auto& self = *this; // be able to modify members
		self.tiles.clear();
		self.edges.clear();
		self.vertices.clear();
		self.tileEdges.clear();
		self.edgeVertices.clear();

        std::unordered_map<size_t, Vertex> verticesMap;
        std::unordered_map<size_t, Edge> edgesMap;

        for (auto it = j.begin(); it != j.end(); ++it) {
       		size_t tileId = std::stoul(it.key());
         	const json& tileJson = it.value();

          	// TODO: add robust input validation
          	if (!tileJson.contains("edges") || !tileJson["edges"].is_object()) {
           		throw std::runtime_error("Invalid JSON structure");
           	}

          	Tile tile;
          	tile.setId(tileId);

           	tile.deserialize(tileJson["meta"]);
           	self.tiles.push_back(tile);

            const auto& edgesJson = tileJson["edges"];

            std::array<Edge, 6> tileEdgesArray{};
            size_t edgeIndex = 0;

            for (auto edgeIt = edgesJson.begin(); edgeIt != edgesJson.end(); ++edgeIt) {
            	if (!edgeIt.value().is_array() || edgeIt.value().size() != 2) {
           			throw std::runtime_error("Invalid JSON structure");
             	}

           		size_t edgeId = std::stoul(edgeIt.key());
             	const json& verticesJson = edgeIt.value();

              	Edge edge{edgeId};
               	edgesMap.emplace(edgeId, edge);

                size_t vId0 = verticesJson.at(0).get<size_t>();
                size_t vId1 = verticesJson.at(1).get<size_t>();

                Vertex v0{vId0};
                Vertex v1{vId1};

                verticesMap.emplace(vId0, v0);
                verticesMap.emplace(vId1, v1);

                self.edgeVertices[edgeId] = { verticesMap[vId0], verticesMap[vId1] };

                if (edgeIndex < 6) { tileEdgesArray[edgeIndex++] = edgesMap.at(edgeId); }
            }

            self.tileEdges[tileId] = tileEdgesArray;
        }

        for (auto& [id, vertex] : verticesMap) { self.vertices.push_back(vertex); }
        for (auto& [id, edge] : edgesMap) { self.edges.push_back(edge); }
    }


    /**
     * Serialized the graph topology and stores it in json format into the specified file location.
     */
    void Graph::save(std::filesystem::path& to) {
        std::ofstream file(to);

        if (!file.is_open()) { throw std::runtime_error("Failed to open file for writing"); }

        file << this->serialize().dump(4);
        file.close();
    }


    /**
     * Reads data from the specified file and deserializes it into the graph data type.
     */
    void Graph::load(std::filesystem::path& from) {
        std::ifstream file(from);

        if (!file.is_open()) { throw std::runtime_error("Failed to open file for reading"); }

        std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        this->deserialize(data);
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
                if (edge.getId() != SIZE_MAX) {
                	neighbors.push_back(edge.getId());
                }
            }
        }
        if (auto it = this->tileVertices.find(id); it != this->tileVertices.end()) {
            for (const auto& vertex : it->second) {
                if (vertex.getId() != SIZE_MAX) {
                	neighbors.push_back(vertex.getId());
                }
            }
        }

        // Check for id being of an edge:
        if (auto it = this->edgeVertices.find(id); it != this->edgeVertices.end()) {
            for (const auto& vertex : it->second) {
                if (vertex.getId() != SIZE_MAX) {
                	neighbors.push_back(vertex.getId());
                }
            }
        }

        // Check for id being of a vertex:
        if (auto it = this->vertexEdges.find(id); it != this->vertexEdges.end()) {
            for (const auto& edge : it->second) {
                if (edge.getId() != SIZE_MAX) {
                	neighbors.push_back(edge.getId());
                }
            }
        }
        if (auto it = this->vertexTiles.find(id); it != this->vertexTiles.end()) {
            for (const auto& tile : it->second) {
                if (tile.getId() != SIZE_MAX) {
                	neighbors.push_back(tile.getId());
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
    template <HasIdProperty T>
    std::vector<T> Graph::breadthFirstSearch(const T& start) const {
        std::vector<T> sequence;
        std::unordered_set<size_t> visitedIds;
        std::queue<size_t> q;

        q.push(start.getId());
        visitedIds.insert(start.getId());

        while (!q.empty()) {
            const size_t currentId = q.front();
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
    template <HasIdProperty T>
    std::vector<T> Graph::depthFirstSearch(const T& start) const {
        std::vector<T> sequence;
        std::unordered_set<size_t> visited;
        std::stack<size_t> s;

        s.push(start.getId());

        while (!s.empty()) {
            const size_t currentId = s.top();
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


    // works for weighted graphs -> could be useful later when different terrain yields different
    // difficulties for travel. Also rule-based AI might use this for building roads and stuff...
    // Algorithm calculates shortest paths to nodes *of same type*; TODO: need to think about this more
    template<HasIdProperty T>
    std::vector<T> Graph::dijkstra(const T& start) const {
   		constexpr double INF = std::numeric_limits<double>::infinity();

     	// map nodeId -> current best distance
     	std::unordered_map<size_t, double> distance;
      	std::unordered_map<size_t, size_t> previous;

		// regard only certain type of nodes -> decide what T actually is
		const std::vector<T>& nodes = [this]() -> const std::vector<T>& {
			if constexpr (std::is_same_v<T, Tile>) {
				return this->tiles;
			}
			else if constexpr (std::is_same_v<T, Edge>) {
				return this->edges;
			}
			else {
				return this->vertices;
			}
		}(); //directly call lambda

		for (const auto& node : nodes) {
			distance[node.getId()] = INF; // make sure T has id
			previous[node.getId()] = SIZE_MAX;
		}

		distance[start.getId()] = 0.0; // distance with itfelf

		auto cmp = [](const std::pair<double, size_t>& a, const std::pair<double, size_t>& b) {
			return a.first > b.first;
		};

		std::priority_queue<std::pair<double, size_t>, std::vector<std::pair<double, size_t>>, decltype(cmp)> q(cmp);
		q.emplace(0.0, start.getId());

		while (!q.empty()) {
			auto [dist, currentId] = q.top();
			q.pop();

			if (dist > distance[currentId]) {
				continue;
			}

			for (size_t neighbourId : this->getNeighborIds(currentId)) {
				double alternative = dist + 1.0; // fixed weight

				if (alternative < distance[neighbourId]) {
					distance[neighbourId] = alternative;
					previous[neighbourId] = currentId;
					q.emplace(alternative, neighbourId);
				}
			}
		}

		std::vector<T> reachableNodes;

		for (const auto& node : nodes) {
			if (distance[node.getId()] < INF) {
				reachableNodes.push_back(node);
			}
		}

		return reachableNodes;
    }

    // Get the distance between two nodes (of same type) using basic BFS implementaiton
    template<HasIdProperty T>
    size_t Graph::getDistanceBetween(const T& start, const T& end) const {
        const size_t startId = start.getId();
        const size_t endId = end.getId();

        if (startId == endId) return 0;

        std::queue<size_t> q;
        std::unordered_map<size_t, size_t> distances;
        std::unordered_set<size_t> visited;

        q.push(startId);
        visited.insert(startId);
        distances[startId] = 0;

        while (!q.empty()) {
            size_t currentId = q.front();
            q.pop();

            for (size_t neighbourId : this->getNeighborIds(currentId)) {
                if (visited.find(neighbourId) == visited.end()) {
                    visited.insert(neighbourId);
                    distances[neighbourId] = distances[currentId] + 1;

                    if (neighbourId == endId) {
                        return distances[neighbourId];
                    }

                    q.push(neighbourId);
                }
            }
        }

        return SIZE_MAX; // no path
    }

    // Explicit template instantiation for Tile (currently only used for Tile)
    template size_t Graph::getDistanceBetween<Tile>(const Tile& start, const Tile& end) const;

    // Map methods
	void Graph::regenerate(const WorldGeneratorConfig &worldGeneratorConfig) {
		if (const Result<std::vector<Tile>, ResultError> generatedTiles = WorldGenerator::generateTiles(worldGeneratorConfig); generatedTiles.isOk()) {
			setMapWidth(worldGeneratorConfig.columns);
			/*tiles.clear();
			for (const Tile& tile : generatedTiles.unwrap<>()) {
				addTile(tile);
			}*/
			tiles = generatedTiles.unwrap();
			fmt::print("Graph::regenerate: Map pointer: {}\n", reinterpret_cast<uintptr_t>(this));
			fmt::print("Graph::regenerate: Map width: {}\n", worldGeneratorConfig.columns);
			fmt::print("Graph::regenerate: Tile count: {}\n", getTileCount());
			this->renderUpdateRequested = true;
		} else {
			std::cerr << generatedTiles.unwrapErr() << std::endl;
		}
	}
}
