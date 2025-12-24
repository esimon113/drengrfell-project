#include "graph.h"

#include <cstdint>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <algorithm>
#include <queue>
#include <fstream>

#include "fmt/base.h"
#include "worldGenerator.h"


namespace df {
	void Graph::addTile(std::unique_ptr<Tile> tile) {
		if (std::find(this->tiles.begin(), this->tiles.end(), tile) == this->tiles.end()) {
			this->tiles.push_back(std::move(tile));

			std::array<EdgeHandle, 6> edgesToAdd{};
			std::array<VertexHandle, 6> verticesToAdd{};

			// added with default values (from constructors)
			this->tileEdges[tile->getId()] = edgesToAdd;
			this->tileVertices[tile->getId()] = verticesToAdd;
		}
	}


	void Graph::addEdge(std::unique_ptr<Edge> edge) {
		if (std::find(this->edges.begin(), this->edges.end(), edge) == this->edges.end()) {
			this->edges.push_back(std::move(edge));

			std::array<VertexHandle, 2> verticesToAdd{};

			this->edgeVertices[edge->getId()] = verticesToAdd;
		}
	}


	void Graph::addVertex(std::unique_ptr<Vertex> vertex) {
		if (std::find(this->vertices.begin(), this->vertices.end(), vertex) == this->vertices.end()) {
			this->vertices.push_back(std::move(vertex));

			std::array<EdgeHandle, 3> edgesToAdd{};
			std::array<TileHandle, 3> tilesToAdd{};

			this->vertexEdges[vertex->getId()] = edgesToAdd;
			this->vertexTiles[vertex->getId()] = tilesToAdd;
		}
	}


	// TODO: use optional here?!
	// Throws out_of_range if no tile with id
	TileHandle Graph::getTile(size_t index) const {
		if (!this->doesTileExist(index)) {
			throw std::out_of_range("Tile index out of range");
		}

		return this->tiles[index].get();
	}


	// Throws out_of_range if no edge with id
	EdgeHandle Graph::getEdge(size_t index) const {
		if (!this->doesEdgeExist(index)) {
			throw std::out_of_range("Edge index out of range");
		}

		return this->edges[index].get();
	}


	// Throws out_of_range if no vertex with id
	VertexHandle Graph::getVertex(size_t index) const {
		if (!this->doesTileExist(index)) {
			throw std::out_of_range("Vertex index out of range");
		}

		return this->vertices[index].get();
	}


	bool Graph::doesTileExist(const TileHandle tile) const {
		if (!tile) return false;

		return std::any_of(
			this->tiles.begin(),
			this->tiles.end(),
			[&](const std::unique_ptr<Tile>& t) { return t.get() == tile; }
		);
	}

	bool Graph::doesTileExist(size_t tileId) const {
		return tileId < this->tiles.size() && this->tiles[tileId] != nullptr;
	}


	bool Graph::doesEdgeExist(const EdgeHandle edge) const {
		if (!edge) return false;

		return std::any_of(
			this->edges.begin(),
			this->edges.end(),
			[&](const std::unique_ptr<Edge>& e) { return e.get() == edge; }
		);
	}


	bool Graph::doesEdgeExist(size_t edgeId) const {
		return edgeId < this->edges.size() && this->edges[edgeId] != nullptr;
	}


	bool Graph::doesVertexExist(const VertexHandle vertex) const {
		if (!vertex) return false;

		return std::any_of(
			this->vertices.begin(),
			this->vertices.end(),
			[&](const std::unique_ptr<Vertex>& v) { return v.get() == vertex; }
		);
	}


	bool Graph::doesVertexExist(size_t vertexId) const {
		return vertexId < this->vertices.size() && this->vertices[vertexId] != nullptr;
	}


	void Graph::removeTile(const TileHandle tile) {
		if (!this->doesTileExist(tile)) return;

		auto it = std::find_if(
			this->tiles.begin(),
			this->tiles.end(),
			[&](const std::unique_ptr<Tile>& t) { return t.get() == tile; }
		);
		if (it == this->tiles.end()) return;

		const size_t tileId = it->get()->getId();

		this->tileEdges.erase(tileId);
		this->tileVertices.erase(tileId);
		this->tiles.erase(it);
	}


	void Graph::removeEdge(const EdgeHandle edge) {
		if (!this->doesEdgeExist(edge)) return;

		auto it = std::find_if(
			this->edges.begin(),
			this->edges.end(),
			[&](const std::unique_ptr<Edge>& e) { return e.get() == edge; }
		);
		if (it == this->edges.end()) return;

		size_t edgeId = it->get()->getId();

		this->edgeVertices.erase(edgeId);
		this->edges.erase(it);
	}


	void Graph::removeVertex(const VertexHandle vertex) {
		if (!this->doesVertexExist(vertex)) return;

		auto it = std::find_if(
			this->vertices.begin(),
			this->vertices.end(),
			[&](const std::unique_ptr<Vertex>& v) { return v.get() == vertex; }
		);
		if (it == this->vertices.end()) return;

		size_t vertexId = it->get()->getId();

		this->vertexEdges.erase(vertexId);
		this->vertexTiles.erase(vertexId);
		this->vertices.erase(it);
	}


	void Graph::connectEdgeToTile(const TileHandle tile, const EdgeHandle edge) {
		if (!this->doesTileExist(tile)) return;
		if (!this->doesEdgeExist(edge)) return;

		auto& localEdges = this->tileEdges[tile->getId()];
		for (size_t i = 0; i < 6; ++i) {
			if (localEdges[i]->getId() == SIZE_MAX) {
				localEdges[i] = edge;
				break;
			}
		}
	}


	void Graph::connectVertexToEdge(const EdgeHandle edge, const VertexHandle vertex) {
		if (!this->doesEdgeExist(edge)) return;
		if (!this->doesVertexExist(vertex)) return;

		auto& localVertices = this->edgeVertices[edge->getId()];
		for (size_t i = 0; i < 2; ++i) {
			if (localVertices[i]->getId() == SIZE_MAX) {
				localVertices[i] = vertex;
				break;
			}
		}
	}


	void Graph::connectVertexToTile(const VertexHandle vertex, const TileHandle tile) {
		if (!this->doesVertexExist(vertex)) return;
		if (!this->doesTileExist(tile)) return;

		auto& localVertices = this->tileVertices[tile->getId()];
		for (size_t i = 0; i < 6; ++i) {
			if (localVertices[i]->getId() == SIZE_MAX) {
				localVertices[i] = vertex;
				break;
			}
		}
	}


	/**
	* Returns std::nullopt if the tile is not found.
	*/
	std::optional<std::array<EdgeHandle, 6>> Graph::getTileEdges(const TileHandle tile) const {
		if (!this->doesTileExist(tile)) return std::nullopt;

		auto it = this->tileEdges.find(tile->getId());
		if (it != this->tileEdges.end()) return it->second;

		return std::nullopt;
	}


	/**
	* Returns std::nullopt if the tile is not found.
	*/
	std::optional<std::array<VertexHandle, 6>> Graph::getTileVertices(const TileHandle tile) const {
		if (!this->doesTileExist(tile)) return std::nullopt;

		auto it = this->tileVertices.find(tile->getId());
		if (it != this->tileVertices.end()) return it->second;

		return std::nullopt;
	}


	/**
	* Returns std::nullopt if the edge is not found.
	*/
	std::optional<std::array<VertexHandle, 2>> Graph::getEdgeVertices(const EdgeHandle edge) const {
		if (!this->doesEdgeExist(edge)) return std::nullopt;

		auto it = this->edgeVertices.find(edge->getId());
		if (it != this->edgeVertices.end()) return it->second;

		return std::nullopt;
	}


	/**
	* Returns std::nullopt if the vertex is not found.
	*/
	std::optional<std::array<EdgeHandle, 3>> Graph::getVertexEdges(const VertexHandle vertex) const {
		if (!this->doesVertexExist(vertex)) return std::nullopt;

		auto it = this->vertexEdges.find(vertex->getId());
		if (it != this->vertexEdges.end()) return it->second;

		return std::nullopt;
	}


	/**
	* Returns std::nullopt if the vertex is not found.
	*/
	std::optional<std::array<TileHandle, 3>> Graph::getVertexTiles(const VertexHandle vertex) const {
		if (!this->doesVertexExist(vertex)) return std::nullopt;

		auto it = this->vertexTiles.find(vertex->getId());
		if (it != this->vertexTiles.end()) return it->second;

		return std::nullopt;
	}


	// get the edge index (0-5) by the "global" edgeId
	size_t Graph::getEdgeIndex(size_t edgeId) {
		if (!this->doesEdgeExist(edgeId)) return SIZE_MAX;

		for (const auto& tile : this->tiles) {
			if (auto localTileEdgesOpt = this->getTileEdges(tile.get()); localTileEdgesOpt) {
				auto& localTileEdges = *localTileEdgesOpt;
				auto it = std::ranges::find_if(
					localTileEdges,
					[&](EdgeHandle e) { return e->getId() == edgeId; }
				);

				if (it != localTileEdges.end()) return std::distance(localTileEdges.begin(), it);
			}
		}
		return SIZE_MAX;
	}


	json Graph::serialize() const {
		json j;

		for (const auto& tile : this->tiles) {
			json tileJson;
			tileJson["id"] = tile->getId();
			tileJson["meta"] = tile->serialize();

			json edgesJson;
			if (this->tileEdges.contains(tile->getId())) {
				for (const auto& edge : this->tileEdges.at(tile->getId())) {
					if (this->edgeVertices.contains(edge->getId())) {
						const auto& v = this->edgeVertices.at(edge->getId());
						edgesJson[std::to_string(edge->getId())] = { v.at(0)->getId(), v.at(1)->getId() };
					} else {
						fmt::println("Edge vertices not found for edge {}", edge->getId());
					}
				}
			} else {
				fmt::println("Tile edges not found for tile {}", tile->getId());
			}

			tileJson["edges"] = edgesJson;
			j[std::to_string(tile->getId())] = tileJson;
		}

		return j;
	}


	// TODO: CHECK / FIX THIS
	// also: add error handling + sanity checks
	// -> currently assumes correct JSON structure
	// can throw...
	void Graph::deserialize(const std::string& data) {
		json j = json::parse(data);

		auto& self = *this; // be able to modify members
		self.tiles.clear();
		self.edges.clear();
		self.vertices.clear();
		self.tileEdges.clear();
		self.edgeVertices.clear();

		std::unordered_map<size_t, VertexHandle> verticesMap;
		std::unordered_map<size_t, EdgeHandle> edgesMap;

		for (auto it = j.begin(); it != j.end(); ++it) {
			size_t tileId = std::stoul(it.key());
			const json& tileJson = it.value();

			// TODO: add robust input validation
			if (!tileJson.contains("edges") || !tileJson["edges"].is_object()) {
				throw std::runtime_error("Invalid JSON structure");
			}

			std::unique_ptr<Tile> tile = std::make_unique<Tile>();
			tile->setId(tileId);
			tile->deserialize(tileJson["meta"]);
			self.addTile(std::move(tile));

			const auto& edgesJson = tileJson["edges"];

			std::array<EdgeHandle, 6> tileEdgesArray{};
			size_t edgeIndex = 0;

			for (auto edgeIt = edgesJson.begin(); edgeIt != edgesJson.end(); ++edgeIt) {
				if (!edgeIt.value().is_array() || edgeIt.value().size() != 2) {
					throw std::runtime_error("Invalid JSON structure");
				}

				size_t edgeId = std::stoul(edgeIt.key());
				const json& verticesJson = edgeIt.value();

				Edge edge;
				edge.setId(edgeId);
				edgesMap.emplace(edgeId, &edge);

				size_t vId0 = verticesJson.at(0).get<size_t>();
				size_t vId1 = verticesJson.at(1).get<size_t>();

				Vertex v0(vId0);
				Vertex v1(vId1);

				verticesMap.emplace(vId0, &v0);
				verticesMap.emplace(vId1, &v1);

				self.edgeVertices[edgeId] = { verticesMap[vId0], verticesMap[vId1] };
				if (edgeIndex < 6) { tileEdgesArray[edgeIndex++] = edgesMap.at(edgeId); }
			}

			self.tileEdges[tileId] = tileEdgesArray;
		}
		// TODO:
		// for (auto& [id, vertex] : verticesMap) { self.vertices.push_back(vertex); }
		// for (auto& [id, edge] : edgesMap) { self.edges.push_back(edge); }
	}


	/**
	* Serialized the graph topology and stores it in json format into the specified file location.
	* Throws if file could not be opened.
	*/
	void Graph::save(std::filesystem::path& to) {
		std::ofstream file(to);

		if (!file.is_open()) { throw std::runtime_error("Failed to open file for writing"); }

		file << this->serialize().dump(4);
		file.close();
	}


	/**
	* Reads data from the specified file and deserializes it into the graph data type.
	* Throws if file could not be opened.
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
				if (edge->getId() != SIZE_MAX) {
					neighbors.push_back(edge->getId());
				}
			}
		}
		if (auto it = this->tileVertices.find(id); it != this->tileVertices.end()) {
			for (const auto& vertex : it->second) {
				if (vertex->getId() != SIZE_MAX) {
					neighbors.push_back(vertex->getId());
				}
			}
		}

		// Check for id being of an edge:
		if (auto it = this->edgeVertices.find(id); it != this->edgeVertices.end()) {
			for (const auto& vertex : it->second) {
				if (vertex->getId() != SIZE_MAX) {
					neighbors.push_back(vertex->getId());
				}
			}
		}

		// Check for id being of a vertex:
		if (auto it = this->vertexEdges.find(id); it != this->vertexEdges.end()) {
			for (const auto& edge : it->second) {
				if (edge->getId() != SIZE_MAX) {
					neighbors.push_back(edge->getId());
				}
			}
		}

		if (auto it = this->vertexTiles.find(id); it != this->vertexTiles.end()) {
			for (const auto& tile : it->second) {
				if (tile->getId() != SIZE_MAX) {
					neighbors.push_back(tile->getId());
				}
			}
		}

		return neighbors;
	}


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
			// tiles = generatedTiles.unwrap();
			this->initializeTilesForGraph(generatedTiles.unwrap());
			try { this->populate(); }
			catch (const std::exception& e) { std::cerr << "Error populating graph: " << e.what() << std::endl; }
			this->renderUpdateRequested = true;
		} else {
			std::cerr << generatedTiles.unwrapErr() << std::endl;
		}
	}


	void Graph::initializeTilesForGraph(std::vector<Tile> newTiles) {
		if (newTiles.empty()) return;

		for (auto newTile : newTiles) {
			std::unique_ptr<Tile> tile = std::make_unique<Tile>(newTile.getId(), newTile.getType(), newTile.getPotency());
			this->addTile(std::move(tile));
		}
	}


	// populates the graph with edges and vertices for all tiles.
	// this function also regards the fact that some tiles share edges and/or vertices
	void Graph::populate() {
		if (this->tiles.empty() || this->mapWidth == 0) return;

		this->edges.clear();
		this->vertices.clear();
		this->tileEdges.clear();
		this->tileVertices.clear();
		this->edgeVertices.clear();
		this->vertexEdges.clear();
		this->vertexTiles.clear();

		const size_t columns = this->mapWidth;
		const size_t rows = this->tiles.size() / columns;

		// use hash as map key
		struct PairHash {
			size_t operator()(const std::pair<size_t, size_t>& p) const {
				return std::hash<size_t>{}(p.first) ^ (std::hash<size_t>{}(p.second) << 1);
			}
		};

		// track created edges/vertexes to avoid duplicates
		// format: key=(canonical tile id, index), value=id for vertex/edge
		// canonical tile id: smallest tile id that shares this element
		std::unordered_map<std::pair<size_t, size_t>, size_t, PairHash> edgeIdMap;
		std::unordered_map<std::pair<size_t, size_t>, size_t, PairHash> vertexIdMap;

		// helper to get neighbouring tile id at some offset (dRow, dCol)
		auto getNeighbour = [columns, rows, this](size_t tileId, int dRow, int dCol) -> std::optional<size_t> {
			size_t row = tileId / columns;
			size_t col = tileId % columns;
			int newRow = static_cast<int>(row) + dRow;
			int newCol = static_cast<int>(col) + dCol;

			// Check bounds
			if (newRow < 0 || newRow >= static_cast<int>(rows) || newCol < 0 || newCol >= static_cast<int>(columns)) return std::nullopt;

			size_t neighbourId = static_cast<size_t>(newRow) * columns + static_cast<size_t>(newCol);
			return (neighbourId < this->tiles.size()) ? std::optional<size_t>(neighbourId) : std::nullopt;
		};

		// make sure shared vertices (by tiles) get same id -> no duplicats
		// use "canonical key" for a vertex -> "refrence" tile with smallest id
		auto getVertexKey = [&getNeighbour, columns](size_t tileId, size_t vertexIndex) -> std::pair<size_t, size_t> {
			size_t row = tileId / columns;
			bool isOdd = (row & 1) == 1; //odd rows have different neighbours -> hexagonal represented by row/col
			size_t minTileId = tileId;
			std::vector<size_t> sharingTiles = { tileId };

			switch (vertexIndex) {
				case 0: // Top
					if (isOdd) {
						// Odd row: top-left (row-1, col), top-right (row-1, col+1)
						if (auto n = getNeighbour(tileId, -1, 0)) sharingTiles.push_back(*n);
						if (auto n = getNeighbour(tileId, -1, 1)) sharingTiles.push_back(*n);
					} else {
						// Even row: top-left (row-1, col-1), top-right (row-1, col)
						if (auto n = getNeighbour(tileId, -1, -1)) sharingTiles.push_back(*n);
						if (auto n = getNeighbour(tileId, -1, 0)) sharingTiles.push_back(*n);
					}
					break;
				case 1: // Top-right
					if (auto n = getNeighbour(tileId, 0, 1)) sharingTiles.push_back(*n);
					break;
				case 2: // Bottom-right -> right flat edge (sharde with right neighbour)
					if (auto n = getNeighbour(tileId, 0, 1)) sharingTiles.push_back(*n);
					break;
				case 3: // Bottom vertex
					if (isOdd) {
						// Odd row: bottom-left (row+1, col), bottom-right (row+1, col+1)
						if (auto n = getNeighbour(tileId, 1, 0)) sharingTiles.push_back(*n);
						if (auto n = getNeighbour(tileId, 1, 1)) sharingTiles.push_back(*n);
					} else {
						// Even row: bottom-left (row+1, col-1), bottom-right (row+1, col)
						if (auto n = getNeighbour(tileId, 1, -1)) sharingTiles.push_back(*n);
						if (auto n = getNeighbour(tileId, 1, 0)) sharingTiles.push_back(*n);
					}
					break;
				case 4: //bottom-left -> shared with left neighbuor
					if (auto n = getNeighbour(tileId, 0, -1)) sharingTiles.push_back(*n);
					break;
				case 5: // top-left vertex ->left neighboor
					if (auto n = getNeighbour(tileId, 0, -1)) sharingTiles.push_back(*n);
					break;
			}

			// get canconical tile
			for (size_t tid : sharingTiles) if (tid < minTileId) minTileId = tid;

			// (canonicalTileId, vertexIndex)
			return { minTileId, vertexIndex };
		};

		// similar to above, just for edges (shard among at most 2 tiles)
		auto getEdgeKey = [&getNeighbour, columns](size_t tileId, size_t edgeIndex) -> std::pair<size_t, size_t> {
			size_t row = tileId / columns;
			bool isOdd = (row & 1) == 1;
			size_t minTileId = tileId;
			std::optional<size_t> neighbour;

			switch (edgeIndex) {
				case 0:
					neighbour = isOdd ? getNeighbour(tileId, -1, 1) : getNeighbour(tileId, -1, 0);
					break;
				case 1:
					neighbour = getNeighbour(tileId, 0, 1);
					break;
				case 2:
					neighbour = isOdd ? getNeighbour(tileId, 1, 1) : getNeighbour(tileId, 1, 0);
					break;
				case 3:
					neighbour = isOdd ? getNeighbour(tileId, 1, 0) : getNeighbour(tileId, 1, -1);
					break;
				case 4:
					neighbour = getNeighbour(tileId, 0, -1);
					break;
				case 5:
					neighbour = isOdd ? getNeighbour(tileId, -1, 0) : getNeighbour(tileId, -1, -1);
					break;
			}

			if (neighbour && *neighbour < minTileId) minTileId = *neighbour;

			return { minTileId, edgeIndex };
		};

		// Calculate the maximum tile ID to avoid ID conflicts
		// Vertex IDs will start at maxTileId + 1, edge IDs at maxTileId + 1000000 -> TODO: make more flexible (although this sould be enough)
		size_t maxTileId = 0;
		for (const auto& tile : this->tiles) if (tile->getId() > maxTileId) maxTileId = tile->getId();

		size_t nextVertexId = maxTileId + 1;
		size_t nextEdgeId = maxTileId + 1000000;

		for (const auto& tile : this->tiles) {
			size_t tileId = tile->getId();
			std::array<EdgeHandle, 6> tileEdgesArray{};
			std::array<VertexHandle, 6> tileVerticesArray{};

			for (size_t vi = 0; vi < 6; ++vi) {
				// canonical key for vertex
				auto key = getVertexKey(tileId, vi);
				VertexHandle tmpVertex = nullptr;
				size_t vertexId;

				// if already exists, use existing vertex, else add unique new one
				if (auto it = vertexIdMap.find(key); it != vertexIdMap.end()) {
					vertexId = it->second;
					try {
						tmpVertex = this->getVertex(vertexId);
					} catch (std::exception& e) {
						fmt::println("Error while getting vertex by id ({}): {}", vertexId, e.what());
						continue;
					}
				} else {
					vertexId = nextVertexId++;

					auto vertex = std::make_unique<Vertex>(vertexId);
					tmpVertex = vertex.get();
					this->addVertex(std::move(vertex));

					vertexIdMap[key] = vertexId;
				}

				// store in tileVertices array and connect to tile
				tileVerticesArray[vi] = tmpVertex;
				this->connectVertexToTile(tmpVertex, tile.get());
			}

			// simliar to above
			for (size_t ei = 0; ei < 6; ++ei) {
				auto key = getEdgeKey(tileId, ei);
				EdgeHandle tmpEdge = nullptr;
				size_t edgeId;

				if (auto it = edgeIdMap.find(key); it != edgeIdMap.end()) {
					edgeId = it->second;
					try {
						tmpEdge = this->getEdge(edgeId);
					} catch (std::exception& e) {
						fmt::println("Error while getting edge by id ({}): {}", edgeId, e.what());
						continue;
					}
				} else {
					edgeId = nextEdgeId++;

					auto edge = std::make_unique<Edge>(edgeId);
					tmpEdge = edge.get();
					this->addEdge(std::move(edge));

					edgeIdMap[key] = edgeId;
				}

				tileEdgesArray[ei] = tmpEdge;
				this->connectEdgeToTile(tile.get(), tmpEdge);

				//connect edge to the 2 vertices
				// Edge i connects vertex i to vertex (i+1) % 6
				auto vertex1Key = getVertexKey(tileId, ei);
				auto vertex2Key = getVertexKey(tileId, (ei + 1) % 6);
				this->connectVertexToEdge(tmpEdge, this->getVertex(vertexIdMap[vertex1Key]));
				this->connectVertexToEdge(tmpEdge, this->getVertex(vertexIdMap[vertex2Key]));
			}

			// store for this tile
			this->tileEdges[tileId] = tileEdgesArray;
			this->tileVertices[tileId] = tileVerticesArray;
		}

		// populate reverse lookup maps
		for (const auto& vertex : this->vertices) {
			size_t vertexId = vertex->getId();
			std::array<EdgeHandle, 3> vertexEdgesArray{};
			std::array<TileHandle, 3> vertexTilesArray{};
			size_t edgeCount = 0;
			size_t tileCount = 0;

			for (const auto& edge : this->edges) {
				if (auto edgeVerticesOpt = this->getEdgeVertices(edge.get()); edgeVerticesOpt) {
					for (const auto& v : *edgeVerticesOpt) {
						if (v->getId() == vertexId && edgeCount < 3) {
							vertexEdgesArray[edgeCount++] = edge.get();
							break;
						}
					}
				}
			}

			for (const auto& tile : this->tiles) {
				if (auto tileVerticesOpt = this->getTileVertices(tile.get()); tileVerticesOpt) {
					for (const auto& v : *tileVerticesOpt) {
						if (v->getId() == vertexId && tileCount < 3) {
							vertexTilesArray[tileCount++] = tile.get();
							break;
						}
					}
				}
			}

			this->vertexEdges[vertexId] = vertexEdgesArray;
			this->vertexTiles[vertexId] = vertexTilesArray;
		}
	}
}
