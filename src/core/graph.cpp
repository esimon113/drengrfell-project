#include "graph.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

#include "fmt/base.h"
#include "vertex.h"
#include "worldGenerator.h"


namespace df {
	void Graph::addTile(std::unique_ptr<Tile> tile) {
		if (!tile)
			return;

		const size_t tileId = tile->getId();
		if (this->doesTileExist(tile->getId()))
			return;

		this->tiles.push_back(std::move(tile));

		this->tileEdges.emplace(tileId, std::array<EdgeHandle, 6>{});
		this->tileVertices.emplace(tileId, std::array<VertexHandle, 6>{});
	}


	void Graph::addEdge(std::unique_ptr<Edge> edge) {
		if (!edge)
			return;

		const size_t edgeId = edge->getId();
		if (this->findEdgeById(edgeId) != nullptr) {
			fmt::println("[DEBUG].[addEdge] edge with ID {} already exists; returning...", edgeId);
			return;
		}

		this->edges.push_back(std::move(edge));

		this->edgeVertices.emplace(edgeId, std::array<VertexHandle, 2>{});
	}


	void Graph::addVertex(std::unique_ptr<Vertex> vertex) {
		if (!vertex) {
			fmt::println("[DEBUG].[addVertex] vertex is null, returning");
			return;
		}

		const size_t vertexId = vertex->getId();
		fmt::println("[DEBUG].[addVertex] adding vertex with ID: {}", vertexId);
		if (this->findVertexById(vertexId) != nullptr) {
			fmt::println("[DEBUG].[addVertex] vertex with ID {} already exists; returning...", vertexId);
			return;
		}

		fmt::println("[DEBUG].[addVertex] pushing vertex to vector");
		this->vertices.push_back(std::move(vertex));
		fmt::println("[DEBUG].[addVertex] vertex pushed, now emplace in maps");

		this->vertexEdges.emplace(vertexId, std::array<EdgeHandle, 3>{});
		this->vertexTiles.emplace(vertexId, std::array<TileHandle, 3>{});
		fmt::println("[DEBUG].[addVertex] vertex added successfully");
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
		if (!this->doesVertexExist(index)) {
			throw std::out_of_range("Vertex index out of range");
		}

		return this->vertices[index].get();
	}

	// Helper function to find a tile by ID (not index)
	TileHandle Graph::findTileById(size_t tileId) const {
		auto it = std::find_if(
			this->tiles.begin(),
			this->tiles.end(),
			[tileId](const std::unique_ptr<Tile>& t) { return t && t->getId() == tileId; });
		return (it != this->tiles.end()) ? it->get() : nullptr;
	}

	// Helper function to find a vertex by ID (not index)
	VertexHandle Graph::findVertexById(size_t vertexId) const {
		auto it = std::find_if(
			this->vertices.begin(),
			this->vertices.end(),
			[vertexId](const std::unique_ptr<Vertex>& v) { return v && v->getId() == vertexId; });
		return (it != this->vertices.end()) ? it->get() : nullptr;
	}

	// Helper function to find an edge by ID (not index)
	EdgeHandle Graph::findEdgeById(size_t edgeId) const {
		auto it = std::find_if(
			this->edges.begin(),
			this->edges.end(),
			[edgeId](const std::unique_ptr<Edge>& e) { return e && e->getId() == edgeId; });
		return (it != this->edges.end()) ? it->get() : nullptr;
	}


	bool Graph::doesTileExist(const TileHandle tile) const {
		if (!tile)
			return false;

		return std::any_of(
			this->tiles.begin(),
			this->tiles.end(),
			[&](const std::unique_ptr<Tile>& t) { return t.get() == tile; });
	}

	bool Graph::doesTileExist(size_t tileId) const {
		return tileId < this->tiles.size() && this->tiles[tileId] != nullptr;
	}


	bool Graph::doesEdgeExist(const EdgeHandle edge) const {
		if (!edge)
			return false;

		return std::any_of(
			this->edges.begin(),
			this->edges.end(),
			[&](const std::unique_ptr<Edge>& e) { return e.get() == edge; });
	}


	bool Graph::doesEdgeExist(size_t edgeId) const {
		return this->findEdgeById(edgeId) != nullptr;
	}


	bool Graph::doesVertexExist(const VertexHandle vertex) const {
		if (!vertex)
			return false;

		return std::any_of(
			this->vertices.begin(),
			this->vertices.end(),
			[&](const std::unique_ptr<Vertex>& v) { return v.get() == vertex; });
	}


	bool Graph::doesVertexExist(size_t vertexId) const {
		return this->findVertexById(vertexId) != nullptr;
	}


	void Graph::removeTile(const TileHandle tile) {
		if (!this->doesTileExist(tile))
			return;

		auto it = std::find_if(
			this->tiles.begin(),
			this->tiles.end(),
			[&](const std::unique_ptr<Tile>& t) { return t.get() == tile; });
		if (it == this->tiles.end())
			return;

		const size_t tileId = it->get()->getId();

		this->tileEdges.erase(tileId);
		this->tileVertices.erase(tileId);
		this->tiles.erase(it);
	}


	void Graph::removeEdge(const EdgeHandle edge) {
		if (!this->doesEdgeExist(edge))
			return;

		auto it = std::find_if(
			this->edges.begin(),
			this->edges.end(),
			[&](const std::unique_ptr<Edge>& e) { return e.get() == edge; });
		if (it == this->edges.end())
			return;

		size_t edgeId = it->get()->getId();

		this->edgeVertices.erase(edgeId);
		this->edges.erase(it);
	}


	void Graph::removeVertex(const VertexHandle vertex) {
		if (!this->doesVertexExist(vertex))
			return;

		auto it = std::find_if(
			this->vertices.begin(),
			this->vertices.end(),
			[&](const std::unique_ptr<Vertex>& v) { return v.get() == vertex; });
		if (it == this->vertices.end())
			return;

		size_t vertexId = it->get()->getId();

		this->vertexEdges.erase(vertexId);
		this->vertexTiles.erase(vertexId);
		this->vertices.erase(it);
	}


	void Graph::connectEdgeToTile(const TileHandle tile, const EdgeHandle edge) {
		if (!this->doesTileExist(tile))
			return;
		if (!this->doesEdgeExist(edge))
			return;

		auto& localEdges = this->tileEdges[tile->getId()];
		for (size_t i = 0; i < 6; ++i) {
			if (!localEdges[i] || localEdges[i]->getId() == SIZE_MAX) {
				localEdges[i] = edge;
				break;
			}
		}
	}


	void Graph::connectVertexToEdge(const EdgeHandle edge, const VertexHandle vertex) {
		if (!this->doesEdgeExist(edge))
			return;
		if (!this->doesVertexExist(vertex))
			return;

		auto& localVertices = this->edgeVertices[edge->getId()];
		for (size_t i = 0; i < 2; ++i) {
			if (!localVertices[i] || localVertices[i]->getId() == SIZE_MAX) {
				localVertices[i] = vertex;
				break;
			}
		}
	}


	void Graph::connectVertexToTile(const VertexHandle vertex, const TileHandle tile) {
		if (!this->doesVertexExist(vertex))
			return;
		if (!this->doesTileExist(tile))
			return;

		auto& localVertices = this->tileVertices[tile->getId()];
		for (size_t i = 0; i < 6; ++i) {
			if (!localVertices[i] || localVertices[i]->getId() == SIZE_MAX) {
				localVertices[i] = vertex;
				break;
			}
		}
	}


	/**
	 * Returns std::nullopt if the tile is not found.
	 */
	std::optional<std::array<EdgeHandle, 6>> Graph::getTileEdges(const TileHandle tile) const {
		if (!this->doesTileExist(tile))
			return std::nullopt;

		auto it = this->tileEdges.find(tile->getId());
		if (it != this->tileEdges.end())
			return it->second;

		return std::nullopt;
	}


	/**
	 * Returns std::nullopt if the tile is not found.
	 */
	std::optional<std::array<VertexHandle, 6>> Graph::getTileVertices(const TileHandle tile) const {
		if (!this->doesTileExist(tile))
			return std::nullopt;

		auto it = this->tileVertices.find(tile->getId());
		if (it != this->tileVertices.end())
			return it->second;

		return std::nullopt;
	}


	/**
	 * Returns std::nullopt if the edge is not found.
	 */
	std::optional<std::array<VertexHandle, 2>> Graph::getEdgeVertices(const EdgeHandle edge) const {
		if (!this->doesEdgeExist(edge))
			return std::nullopt;

		auto it = this->edgeVertices.find(edge->getId());
		if (it != this->edgeVertices.end())
			return it->second;

		return std::nullopt;
	}


	/**
	 * Returns std::nullopt if the vertex is not found.
	 */
	std::optional<std::array<EdgeHandle, 3>> Graph::getVertexEdges(const VertexHandle vertex) const {
		if (!this->doesVertexExist(vertex))
			return std::nullopt;

		auto it = this->vertexEdges.find(vertex->getId());
		if (it != this->vertexEdges.end())
			return it->second;

		return std::nullopt;
	}


	/**
	 * Returns std::nullopt if the vertex is not found.
	 */
	std::optional<std::array<TileHandle, 3>> Graph::getVertexTiles(const VertexHandle vertex) const {
		if (!this->doesVertexExist(vertex))
			return std::nullopt;

		auto it = this->vertexTiles.find(vertex->getId());
		if (it != this->vertexTiles.end())
			return it->second;

		return std::nullopt;
	}


	// get the edge index (0-5) by the "global" edgeId
	size_t Graph::getEdgeIndex(size_t edgeId) {
		if (!this->doesEdgeExist(edgeId))
			return SIZE_MAX;

		for (const auto& tile : this->tiles) {
			if (auto localTileEdgesOpt = this->getTileEdges(tile.get()); localTileEdgesOpt) {
				auto& localTileEdges = *localTileEdgesOpt;
				auto it = std::ranges::find_if(
					localTileEdges,
					[&](EdgeHandle e) { return e->getId() == edgeId; });

				if (it != localTileEdges.end())
					return std::distance(localTileEdges.begin(), it);
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
						edgesJson[std::to_string(edge->getId())] = {v.at(0)->getId(), v.at(1)->getId()};
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

				self.edgeVertices[edgeId] = {verticesMap[vId0], verticesMap[vId1]};
				if (edgeIndex < 6) {
					tileEdgesArray[edgeIndex++] = edgesMap.at(edgeId);
				}
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

		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file for writing");
		}

		file << this->serialize().dump(4);
		file.close();
	}


	/**
	 * Reads data from the specified file and deserializes it into the graph data type.
	 * Throws if file could not be opened.
	 */
	void Graph::load(std::filesystem::path& from) {
		std::ifstream file(from);

		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file for reading");
		}

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
	template <HasIdProperty T>
	std::vector<T> Graph::dijkstra(const T& start) const {
		constexpr double INF = std::numeric_limits<double>::infinity();

		// map nodeId -> current best distance
		std::unordered_map<size_t, double> distance;
		std::unordered_map<size_t, size_t> previous;

		// regard only certain type of nodes -> decide what T actually is
		const std::vector<T>& nodes = [this]() -> const std::vector<T>& {
			if constexpr (std::is_same_v<T, Tile>) {
				return this->tiles;
			} else if constexpr (std::is_same_v<T, Edge>) {
				return this->edges;
			} else {
				return this->vertices;
			}
		}(); // directly call lambda

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
	template <HasIdProperty T>
	size_t Graph::getDistanceBetween(const T& start, const T& end) const {
		const size_t startId = HasIdPropertyHelper::getId(start);
		const size_t endId = HasIdPropertyHelper::getId(end);

		if (startId == endId)
			return 0;

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


	// Explicit template instantiation for Tile and TileHandle (Tile*)
	template size_t Graph::getDistanceBetween<Tile>(const Tile& start, const Tile& end) const;
	template size_t Graph::getDistanceBetween<TileHandle>(const TileHandle& start, const TileHandle& end) const;


	// Map methods
	void Graph::regenerate(const WorldGeneratorConfig& worldGeneratorConfig) {
		if (const Result<std::vector<Tile>, ResultError> generatedTiles = WorldGenerator::generateTiles(worldGeneratorConfig); generatedTiles.isOk()) {
			fmt::println("[DEBUG] generated tiles");
			setMapWidth(worldGeneratorConfig.columns);
			// tiles = generatedTiles.unwrap();
			fmt::println("[DEBUG] start initializing tiles");
			this->initializeTilesForGraph(generatedTiles.unwrap());
			fmt::println("[DEBUG] finished initializing tiles, try populating graph");
			try {
				this->populate();
			} catch (const std::exception& e) {
				std::cerr << "Error populating graph: " << e.what() << std::endl;
			}
			fmt::println("[DEBUG] finished populating graph");
			this->renderUpdateRequested = true;
		} else {
			std::cerr << generatedTiles.unwrapErr() << std::endl;
		}
	}


	void Graph::initializeTilesForGraph(std::vector<Tile> newTiles) {
		if (newTiles.empty())
			return;
		fmt::println("[DEBUG] start initializing tiles with a non-empty newTiles vector");
		fmt::println("[DEBUG] clear existing tiles");
		this->tiles.clear();
		fmt::println("[DEBUG] iterating over tiles to create new ones");

		for (auto newTile : newTiles) {
			std::unique_ptr<Tile> tile = std::make_unique<Tile>(newTile.getId(), newTile.getType(), newTile.getPotency());
			this->addTile(std::move(tile));
		}
		fmt::println("[DEBUG] finished initializing tiles");
	}


	// populates the graph with edges and vertices for all tiles.
	// this function also regards the fact that some tiles share edges and/or vertices
	void Graph::populate() {
		if (this->tiles.empty() || this->mapWidth == 0)
			return;

		this->edges.clear();
		this->vertices.clear();
		this->tileEdges.clear();
		this->tileVertices.clear();
		this->edgeVertices.clear();
		this->vertexEdges.clear();
		this->vertexTiles.clear();

		fmt::println("[DEBUG].[populate] cleared vectors");

		const size_t columns = this->mapWidth;
		const size_t rows = this->tiles.size() / columns;
		fmt::println("[DEBUG].[populate] received map dimensions: columns={}, rows={}, tiles.size()={}", columns, rows, this->tiles.size());

		if (columns == 0 || rows == 0 || this->tiles.empty()) {
			fmt::println("[DEBUG].[populate] ERROR: Invalid map dimensions or empty tiles!");
			return;
		}

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

		fmt::println("[DEBUG].[populate] start defining some helpers: getNeighbor, getVertexKey, getEdgeKey");
		// helper to get neighbouring tile id at some offset (dRow, dCol)
		fmt::println("[DEBUG].[populate] defining getNeighbour lambda");
		auto getNeighbour = [columns, rows](size_t tileId, int dRow, int dCol) -> std::optional<size_t> {
			size_t row = tileId / columns;
			size_t col = tileId % columns;
			int newRow = static_cast<int>(row) + dRow;
			int newCol = static_cast<int>(col) + dCol;

			// Check bounds
			if (newRow < 0 || newRow >= static_cast<int>(rows) || newCol < 0 || newCol >= static_cast<int>(columns))
				return std::nullopt;

			size_t neighbourId = static_cast<size_t>(newRow) * columns + static_cast<size_t>(newCol);
			// Check if neighbourId is valid and within expected range
			// Note: We can't directly check tiles[neighbourId] because tiles might not be indexed by ID
			// Instead, we just verify the calculated ID is within the expected grid bounds
			if (neighbourId >= rows * columns) {
				return std::nullopt;
			}
			return std::optional<size_t>(neighbourId);
		};
		fmt::println("[DEBUG].[populate] finished defining getNeighbour");

		// make sure shared vertices (by tiles) get same id -> no duplicats
		// use "canonical key" for a vertex -> "refrence" tile with smallest id
		fmt::println("[DEBUG].[populate] defining getVertexKey lambda");
		auto getVertexKey = [&getNeighbour, columns](size_t tileId, size_t vertexIndex) -> std::pair<size_t, size_t> {
			if (columns == 0) {
				throw std::logic_error("getVertexKey: columns is zero!");
			}
			size_t row = tileId / columns;
			bool isOdd = (row & 1) == 1; // odd rows have different neighbours -> hexagonal represented by row/col
			size_t minTileId = tileId;
			std::vector<size_t> sharingTiles = {tileId};

			switch (vertexIndex) {
			case 0: // Top
				if (isOdd) {
					// Odd row: top-left (row-1, col), top-right (row-1, col+1)
					if (auto n = getNeighbour(tileId, -1, 0))
						sharingTiles.push_back(*n);
					if (auto n = getNeighbour(tileId, -1, 1))
						sharingTiles.push_back(*n);
				} else {
					// Even row: top-left (row-1, col-1), top-right (row-1, col)
					if (auto n = getNeighbour(tileId, -1, -1))
						sharingTiles.push_back(*n);
					if (auto n = getNeighbour(tileId, -1, 0))
						sharingTiles.push_back(*n);
				}
				break;
			case 1: // Top-right
				if (auto n = getNeighbour(tileId, 0, 1))
					sharingTiles.push_back(*n);
				break;
			case 2: // Bottom-right -> right flat edge (sharde with right neighbour)
				if (auto n = getNeighbour(tileId, 0, 1))
					sharingTiles.push_back(*n);
				break;
			case 3: // Bottom vertex
				if (isOdd) {
					// Odd row: bottom-left (row+1, col), bottom-right (row+1, col+1)
					if (auto n = getNeighbour(tileId, 1, 0))
						sharingTiles.push_back(*n);
					if (auto n = getNeighbour(tileId, 1, 1))
						sharingTiles.push_back(*n);
				} else {
					// Even row: bottom-left (row+1, col-1), bottom-right (row+1, col)
					if (auto n = getNeighbour(tileId, 1, -1))
						sharingTiles.push_back(*n);
					if (auto n = getNeighbour(tileId, 1, 0))
						sharingTiles.push_back(*n);
				}
				break;
			case 4: // bottom-left -> shared with left neighbuor
				if (auto n = getNeighbour(tileId, 0, -1))
					sharingTiles.push_back(*n);
				break;
			case 5: // top-left vertex ->left neighboor
				if (auto n = getNeighbour(tileId, 0, -1))
					sharingTiles.push_back(*n);
				break;
			}

			// get canconical tile
			for (size_t tid : sharingTiles)
				if (tid < minTileId)
					minTileId = tid;

			// Determine the vertex index in the canonical tile
			// If the canonical tile is the current tile, use the current vertexIndex
			// Otherwise, determine what index this vertex has in the canonical tile
			size_t canonicalVertexIndex = vertexIndex;
			if (minTileId != tileId) {
				std::sort(sharingTiles.begin(), sharingTiles.end());
				bool isCanonicalOdd = ((minTileId / columns) & 1) == 1;

				// For vertices shared by 3 tiles, determine the canonical index
				// by checking the relative positions of sharing tiles
				if (sharingTiles.size() == 3) {

					// Determine which vertex index in the canonical tile corresponds to this shared vertex
					// by checking which neighbours the canonical tile has which match the sharing tiles
					for (size_t vi = 0; vi < 6; ++vi) {
						std::vector<size_t> canonicalSharingTiles = {minTileId};

						switch (vi) {
						case 0: // Top
							if (isCanonicalOdd) {
								if (auto n = getNeighbour(minTileId, -1, 0))
									canonicalSharingTiles.push_back(*n);
								if (auto n = getNeighbour(minTileId, -1, 1))
									canonicalSharingTiles.push_back(*n);
							} else {
								if (auto n = getNeighbour(minTileId, -1, -1))
									canonicalSharingTiles.push_back(*n);
								if (auto n = getNeighbour(minTileId, -1, 0))
									canonicalSharingTiles.push_back(*n);
							}
							break;
						case 1: // Top-right
							if (auto n = getNeighbour(minTileId, 0, 1))
								canonicalSharingTiles.push_back(*n);
							break;
						case 2: // Bottom-right
							if (auto n = getNeighbour(minTileId, 0, 1))
								canonicalSharingTiles.push_back(*n);
							break;
						case 3: // Bottom
							if (isCanonicalOdd) {
								if (auto n = getNeighbour(minTileId, 1, 0))
									canonicalSharingTiles.push_back(*n);
								if (auto n = getNeighbour(minTileId, 1, 1))
									canonicalSharingTiles.push_back(*n);
							} else {
								if (auto n = getNeighbour(minTileId, 1, -1))
									canonicalSharingTiles.push_back(*n);
								if (auto n = getNeighbour(minTileId, 1, 0))
									canonicalSharingTiles.push_back(*n);
							}
							break;
						case 4: // Bottom-left
							if (auto n = getNeighbour(minTileId, 0, -1))
								canonicalSharingTiles.push_back(*n);
							break;
						case 5: // Top-left
							if (auto n = getNeighbour(minTileId, 0, -1))
								canonicalSharingTiles.push_back(*n);
							break;
						}

						std::sort(canonicalSharingTiles.begin(), canonicalSharingTiles.end());
						if (canonicalSharingTiles == sharingTiles) {
							canonicalVertexIndex = vi;
							break;
						}
					}
				}
				// else {
				// 	// For vertices shared by 2 tiles (edge vertices), we need to determine the vertex index in the canonical tile
				// 	// Try to find which vertex index in the canonical tile corresponds to this shared vertex
				// 	// size_t canonicalRow = minTileId / columns;
				// 	// bool canonicalIsOdd = ((minTileId / columns) & 1) == 1;
				//
				// 	for (size_t vi = 0; vi < 6; ++vi) {
				// 		std::vector<size_t> canonicalSharingTiles = {minTileId};
				//
				// 		switch (vi) {
				// 		case 0: // Top
				// 			if (isCanonicalOdd) {
				// 				if (auto n = getNeighbour(minTileId, -1, 0))
				// 					canonicalSharingTiles.push_back(*n);
				// 				if (auto n = getNeighbour(minTileId, -1, 1))
				// 					canonicalSharingTiles.push_back(*n);
				// 			} else {
				// 				if (auto n = getNeighbour(minTileId, -1, -1))
				// 					canonicalSharingTiles.push_back(*n);
				// 				if (auto n = getNeighbour(minTileId, -1, 0))
				// 					canonicalSharingTiles.push_back(*n);
				// 			}
				// 			break;
				// 		case 1: // Top-right
				// 			if (auto n = getNeighbour(minTileId, 0, 1))
				// 				canonicalSharingTiles.push_back(*n);
				// 			break;
				// 		case 2: // Bottom-right
				// 			if (auto n = getNeighbour(minTileId, 0, 1))
				// 				canonicalSharingTiles.push_back(*n);
				// 			break;
				// 		case 3: // Bottom
				// 			if (isCanonicalOdd) {
				// 				if (auto n = getNeighbour(minTileId, 1, 0))
				// 					canonicalSharingTiles.push_back(*n);
				// 				if (auto n = getNeighbour(minTileId, 1, 1))
				// 					canonicalSharingTiles.push_back(*n);
				// 			} else {
				// 				if (auto n = getNeighbour(minTileId, 1, -1))
				// 					canonicalSharingTiles.push_back(*n);
				// 				if (auto n = getNeighbour(minTileId, 1, 0))
				// 					canonicalSharingTiles.push_back(*n);
				// 			}
				// 			break;
				// 		case 4: // Bottom-left
				// 			if (auto n = getNeighbour(minTileId, 0, -1))
				// 				canonicalSharingTiles.push_back(*n);
				// 			break;
				// 		case 5: // Top-left
				// 			if (auto n = getNeighbour(minTileId, 0, -1))
				// 				canonicalSharingTiles.push_back(*n);
				// 			break;
				// 		}
				//
				// 		std::sort(canonicalSharingTiles.begin(), canonicalSharingTiles.end());
				// 		if (canonicalSharingTiles == sharingTiles) {
				// 			canonicalVertexIndex = vi;
				// 			break;
				// 		}
				// 	}
				//
				// 	// If we still couldn't find a match, use a hash of the sorted sharing tiles as a fallback
				// 	// This ensures we still get a consistent key even if the matching logic fails
				// 	if (canonicalVertexIndex == vertexIndex && minTileId != tileId) {
				// 		// Use a simple hash of the sorted sharing tile IDs to create a unique but consistent key
				// 		size_t hash = 0;
				// 		for (size_t tid : sharingTiles) {
				// 			hash = hash * 31 + tid; // Simple hash function
				// 		}
				// 		// Map hash to a vertex index (0-5) using modulo
				// 		canonicalVertexIndex = hash % 6;
				// 	}
				// }
			}

			// (canonicalTileId, canonicalVertexIndex)
			return {minTileId, canonicalVertexIndex};
		};

		// similar to above, just for edges (shard among at most 2 tiles)
		fmt::println("[DEBUG].[populate] defining getEdgeKey lambda");
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

			if (neighbour && *neighbour < minTileId)
				minTileId = *neighbour;

			size_t canonicalEdgeIndex = edgeIndex;
			if (minTileId != tileId && neighbour.has_value()) {
				bool isCanonicalOdd = ((minTileId / columns) & 1) == 1;

				// Which edge index in the canonical tile corresponds to this edge
				// checking which neighbour the canonical tile has which matches the current tile
				for (size_t ei = 0; ei < 6; ++ei) {
					std::optional<size_t> canonicalNeighbour;

					switch (ei) {
					case 0:
						canonicalNeighbour = isCanonicalOdd ? getNeighbour(minTileId, -1, 1) : getNeighbour(minTileId, -1, 0);
						break;
					case 1:
						canonicalNeighbour = getNeighbour(minTileId, 0, 1);
						break;
					case 2:
						canonicalNeighbour = isCanonicalOdd ? getNeighbour(minTileId, 1, 1) : getNeighbour(minTileId, 1, 0);
						break;
					case 3:
						canonicalNeighbour = isCanonicalOdd ? getNeighbour(minTileId, 1, 0) : getNeighbour(minTileId, 1, -1);
						break;
					case 4:
						canonicalNeighbour = getNeighbour(minTileId, 0, -1);
						break;
					case 5:
						canonicalNeighbour = isCanonicalOdd ? getNeighbour(minTileId, -1, 0) : getNeighbour(minTileId, -1, -1);
						break;
					}

					// If the canonical tile's edge connects to the same neighbour (=current tile), that's the correct edge index
					if (canonicalNeighbour.has_value() && *canonicalNeighbour == tileId) {
						canonicalEdgeIndex = ei;
						break;
					}
				}
			}

			return {minTileId, canonicalEdgeIndex};
		};

		// Calculate the maximum tile ID to avoid ID conflicts
		// Vertex IDs will start at maxTileId + 1, edge IDs at maxTileId + 1000000 -> TODO: make more flexible (although this sould be enough)
		size_t maxTileId = 0;
		for (const auto& tile : this->tiles)
			if (tile->getId() > maxTileId)
				maxTileId = tile->getId();
		fmt::print("[DEBUG].[populate] got max tile id: {} and start iterating over tiles", maxTileId);

		size_t nextVertexId = maxTileId + 1;
		size_t nextEdgeId = maxTileId + 1000000;
		fmt::println("[DEBUG].[populate] got max tile id: {} and start iterating over tiles", maxTileId);
		fmt::println("[DEBUG].[populate] about to iterate over {} tiles", this->tiles.size());

		fmt::println("[DEBUG].[populate] starting tile iteration loop");
		size_t tileIndex = 0;
		for (const auto& tile : this->tiles) {
			fmt::println("[DEBUG].[populate] processing tile index {}", tileIndex++);
			if (!tile) {
				fmt::println("[DEBUG].[populate] ERROR: Found null tile pointer!");
				continue;
			}
			fmt::println("[DEBUG].[populate] got tile pointer, getting ID");
			size_t tileId = tile->getId();
			fmt::println("[DEBUG].[populate] tile ID: {}", tileId);
			std::array<EdgeHandle, 6> tileEdgesArray{};
			std::array<VertexHandle, 6> tileVerticesArray{};

			fmt::println("[DEBUG].[populate] starting vertex loop for tile {}", tileId);
			for (size_t vi = 0; vi < 6; ++vi) {
				// canonical key for vertex
				fmt::println("[DEBUG].[populate] calling getVertexKey for tile {}, vertex index {}", tileId, vi);
				auto key = getVertexKey(tileId, vi);
				fmt::println("[DEBUG].[populate] got vertex key: ({}, {})", key.first, key.second);
				VertexHandle tmpVertex = nullptr;

				// if already exists, use existing vertex, else add unique new one
				fmt::println("[DEBUG].[populate] looking up key in vertexIdMap (size: {})", vertexIdMap.size());
				try {
					if (auto it = vertexIdMap.find(key); it != vertexIdMap.end()) {
						fmt::println("[DEBUG].[populate] found existing vertex in map");
						tmpVertex = this->findVertexById(it->second);
						if (!tmpVertex) {
							throw std::logic_error(fmt::format("Error while getting vertex. vertexIdMap out of sync: vertex {} missing", it->second));
							// continue;
						}
					} else {
						fmt::println("[DEBUG].[populate] creating new vertex");
						size_t vertexId = nextVertexId++;
						fmt::println("[DEBUG].[populate] new vertex ID: {}", vertexId);

						auto vertex = std::make_unique<Vertex>(vertexId);
						tmpVertex = vertex.get();
						fmt::println("[DEBUG].[populate] created vertex unique_ptr, calling addVertex");
						this->addVertex(std::move(vertex));
						fmt::println("[DEBUG].[populate] added vertex, storing in map");

						vertexIdMap[key] = vertexId;
						fmt::println("[DEBUG].[populate] stored vertex in map");
					}
				} catch (const std::exception& e) {
					fmt::println("[DEBUG].[populate] EXCEPTION in vertex handling: {}", e.what());
					throw;
				}

				// store in tileVertices array and connect to tile
				tileVerticesArray[vi] = tmpVertex;
				this->connectVertexToTile(tmpVertex, tile.get());

				// Build reverse lookup: add tile to vertexTiles
				size_t vertexId = tmpVertex->getId();
				if (!this->vertexTiles.contains(vertexId)) {
					this->vertexTiles[vertexId] = std::array<TileHandle, 3>{};
				}
				auto& vertexTilesArray = this->vertexTiles[vertexId];
				for (size_t i = 0; i < 3; ++i) {
					if (!vertexTilesArray[i] || vertexTilesArray[i] == tile.get()) {
						vertexTilesArray[i] = tile.get();
						break;
					}
				}
			}
			fmt::println("[DEBUG].[populate] finished with vertices for tile with id: {}", tile->getId());

			// simliar to above
			for (size_t ei = 0; ei < 6; ++ei) {
				auto key = getEdgeKey(tileId, ei);
				EdgeHandle tmpEdge = nullptr;
				size_t edgeId;

				if (auto it = edgeIdMap.find(key); it != edgeIdMap.end()) {
					edgeId = it->second;
					tmpEdge = this->findEdgeById(edgeId);
					if (!tmpEdge) {
						throw std::logic_error(fmt::format("Error while getting edge. edgeIdMap out of sync: edge {} missing", edgeId));
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

				// connect edge to the 2 vertices
				// Edge i connects vertex i to vertex (i+1) % 6
				auto vertex1Key = getVertexKey(tileId, ei);
				auto vertex2Key = getVertexKey(tileId, (ei + 1) % 6);
				auto v1It = vertexIdMap.find(vertex1Key);
				auto v2It = vertexIdMap.find(vertex2Key);
				if (v1It != vertexIdMap.end() && v2It != vertexIdMap.end()) {
					VertexHandle v1 = this->findVertexById(v1It->second);
					VertexHandle v2 = this->findVertexById(v2It->second);
					if (v1 && v2) {
						this->connectVertexToEdge(tmpEdge, v1);
						this->connectVertexToEdge(tmpEdge, v2);

						// Build reverse lookup: add edge to vertexEdges for both vertices
						size_t v1Id = v1->getId();
						size_t v2Id = v2->getId();

						if (!this->vertexEdges.contains(v1Id)) {
							this->vertexEdges[v1Id] = std::array<EdgeHandle, 3>{};
						}
						auto& v1Edges = this->vertexEdges[v1Id];
						for (size_t i = 0; i < 3; ++i) {
							if (!v1Edges[i] || v1Edges[i] == tmpEdge) {
								v1Edges[i] = tmpEdge;
								break;
							}
						}

						if (!this->vertexEdges.contains(v2Id)) {
							this->vertexEdges[v2Id] = std::array<EdgeHandle, 3>{};
						}
						auto& v2Edges = this->vertexEdges[v2Id];
						for (size_t i = 0; i < 3; ++i) {
							if (!v2Edges[i] || v2Edges[i] == tmpEdge) {
								v2Edges[i] = tmpEdge;
								break;
							}
						}
					}
				}
			}
			fmt::println("[DEBUG].[populate] finished with edges");

			// store for this tile
			this->tileEdges[tileId] = tileEdgesArray;
			this->tileVertices[tileId] = tileVerticesArray;
		}

		// Reverse lookup maps (vertexEdges and vertexTiles) are now built during the connection phase above
		// No need for the slow O(V*E + V*T) pass anymore!
		fmt::println("[DEBUG].[populate] finished with populating");
		fmt::println("[DEBUG].[populate] finished wiht populating");
	}
} // namespace df
