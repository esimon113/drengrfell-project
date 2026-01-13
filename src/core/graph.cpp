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

#include "assets.h"
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

	TileHandle Graph::getTileFromWorldPosition(float worldX, float worldY) const {
		int row = static_cast<int>(std::floor((worldY + 0.75f) / 1.5f));

		if (row < 0) {
			fmt::println("Improper world Position: row < 0");
			return nullptr;
		}

		float rowOffset = (row % 2 == 1) ? 1.0f : 0.0f;

		int col = static_cast<int>(std::floor((worldX - rowOffset + 1.0f) / 2.0f));

		if (col < 0) {
			fmt::println("Improper world Position: col < 0");
			return nullptr;
		}

		size_t index = static_cast<size_t>(row) * mapWidth + static_cast<size_t>(col);

		if (index >= tiles.size()) {
			fmt::println("Improper world Position: index out of map bounds");
			return nullptr;
		}

		auto tileType = getTile(index)->getType();
		// auto t = tiles[index].get()->getType();
		std::string tileTypeStr = "";

		switch (tileType) {
		case types::TileType::EMPTY:
			tileTypeStr = "EMPTY";
			break;
		case types::TileType::WATER:
			tileTypeStr = "WATER";
			break;
		case types::TileType::FOREST:
			tileTypeStr = "FOREST";
			break;
		case types::TileType::GRASS:
			tileTypeStr = "GRASS";
			break;
		case types::TileType::MOUNTAIN:
			tileTypeStr = "MOUNTAIN";
			break;
		case types::TileType::FIELD:
			tileTypeStr = "FIELD";
			break;
		case types::TileType::CLAY:
			tileTypeStr = "CLAY";
			break;
		case types::TileType::ICE:
			tileTypeStr = "ICE";
			break;
		default:
			tileTypeStr = "UNKNOWN";
			break;
		}
		fmt::println("tile: {}", tileTypeStr);

		return getTile(index);
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
			// TODO: Place hazards only with certain probabilities, if they should be rendered
			tile->initializeHazardProfile();
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

		fmt::println("[DEBUG].[populate] start defining neighbour helpers");

		// Build lookup from tile id to its position in the grid (row-major order).
		std::unordered_map<size_t, size_t> tileIdToIndex;
		tileIdToIndex.reserve(this->tiles.size());
		for (size_t idx = 0; idx < this->tiles.size(); ++idx) {
			if (this->tiles[idx]) {
				tileIdToIndex[this->tiles[idx]->getId()] = idx;
			}
		}

		// Odd-R offset neighbour deltas (Red Blob style)
		const std::array<std::pair<int, int>, 6> oddOffsets = {
			std::pair{-1, 0}, // NW
			std::pair{-1, 1}, // NE
			std::pair{0, 1},  // E
			std::pair{1, 1},  // SE
			std::pair{1, 0},  // SW
			std::pair{0, -1}  // W
		};
		const std::array<std::pair<int, int>, 6> evenOffsets = {
			std::pair{-1, -1}, // NW
			std::pair{-1, 0},  // NE
			std::pair{0, 1},   // E
			std::pair{1, 0},   // SE
			std::pair{1, -1},  // SW
			std::pair{0, -1}   // W
		};

		// neighbour lookup by tile index (row-major position), independent of tile IDs
		auto getNeighbourIndex = [&](size_t tileIndex, size_t direction) -> std::optional<size_t> {
			const size_t row = tileIndex / columns;
			const size_t col = tileIndex % columns;
			const auto& offsets = (row & 1) ? oddOffsets : evenOffsets;
			int newRow = static_cast<int>(row) + offsets[direction].first;
			int newCol = static_cast<int>(col) + offsets[direction].second;

			if (newRow < 0 || newRow >= static_cast<int>(rows) || newCol < 0 || newCol >= static_cast<int>(columns))
				return std::nullopt;

			const size_t neighbourIndex = static_cast<size_t>(newRow) * columns + static_cast<size_t>(newCol);
			if (neighbourIndex >= this->tiles.size() || !this->tiles[neighbourIndex])
				return std::nullopt;

			return neighbourIndex;
		};

		// Returns which tiles share a vertex, with which vertex index on each neighbor
		// Vertex positions: V0=bottom-right, V1=top-right, V2=top, V3=top-left, V4=bottom-left, V5=bottom
		// Neighbor directions: 0=NW (below-left), 1=NE (below-right), 2=E (right), 3=SE (above-right), 4=SW (above-left), 5=W (left)
		auto buildVertexSharingInfo = [&](size_t tileIndex, size_t vertexIndex) -> std::vector<std::pair<size_t, size_t>> {
			std::vector<std::pair<size_t, size_t>> sharingInfo;
			sharingInfo.push_back({tileIndex, vertexIndex});

			switch (vertexIndex) {
			case 0: // Bottom-right -> connect with neighbour directions: east (to right) and noth-east (below-right)
				if (auto n = getNeighbourIndex(tileIndex, 2); n)
					sharingInfo.push_back({*n, 4}); // E.V4
				if (auto n = getNeighbourIndex(tileIndex, 1); n)
					sharingInfo.push_back({*n, 2}); // NE.V2
				break;
			case 1: // Top-right -> connect with neighbour east and south-east (above-right)
				if (auto n = getNeighbourIndex(tileIndex, 2); n)
					sharingInfo.push_back({*n, 3}); // E.V3
				if (auto n = getNeighbourIndex(tileIndex, 3); n)
					sharingInfo.push_back({*n, 5}); // SE.V5
				break;
			case 2: // Top -> connect  with enighbor south-east and south-west (above-left)
				if (auto n = getNeighbourIndex(tileIndex, 3); n)
					sharingInfo.push_back({*n, 4}); // SE.V4
				if (auto n = getNeighbourIndex(tileIndex, 4); n)
					sharingInfo.push_back({*n, 0}); // SW.V0
				break;
			case 3: // Top-left -> connect with neighbour south-west and west (left)
				if (auto n = getNeighbourIndex(tileIndex, 4); n)
					sharingInfo.push_back({*n, 5}); // SW.V5
				if (auto n = getNeighbourIndex(tileIndex, 5); n)
					sharingInfo.push_back({*n, 1}); // W.V1
				break;
			case 4: // Bottom-left -> connect with neighbour west and north-west (below-left)
				if (auto n = getNeighbourIndex(tileIndex, 5); n)
					sharingInfo.push_back({*n, 0}); // W.V0
				if (auto n = getNeighbourIndex(tileIndex, 0); n)
					sharingInfo.push_back({*n, 2}); // NW.V2
				break;
			case 5: // Bottom -> connect with neighbour north-west and north-east
				if (auto n = getNeighbourIndex(tileIndex, 0); n)
					sharingInfo.push_back({*n, 1}); // NW.V1
				if (auto n = getNeighbourIndex(tileIndex, 1); n)
					sharingInfo.push_back({*n, 3}); // NE.V3
				break;
			}

			return sharingInfo;
		};

		// Return the canonical tile index and its corresponding vertex index
		auto getVertexKey = [&](size_t tileIndex, size_t vertexIndex) -> std::pair<size_t, size_t> {
			auto sharingInfo = buildVertexSharingInfo(tileIndex, vertexIndex);
			if (sharingInfo.empty())
				return {tileIndex, vertexIndex};

			// Sort by tile index to get canonical tile
			std::sort(sharingInfo.begin(), sharingInfo.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
			return sharingInfo.front();
		};

		// Edge i connects vertex i to vertex (i+1)%6.
		// The shared neighbour is the one that shares both vertices of the edge:
		// Edge 0: V0-V1 (right edge) -> E shares V0 and V1 -> direction 2
		// Edge 1: V1-V2 (top-right edge) -> SE shares V1 and V2 -> direction 3
		// Edge 2: V2-V3 (top-left edge) -> SW shares V2 and V3 -> direction 4
		// Edge 3: V3-V4 (left edge) -> W shares V3 and V4 -> direction 5
		// Edge 4: V4-V5 (bottom-left edge) -> NW shares V4 and V5 -> direction 0
		// Edge 5: V5-V0 (bottom-right edge) -> NE shares V5 and V0 -> direction 1
		const std::array<size_t, 6> edgeDirections = {2, 3, 4, 5, 0, 1}; // E, SE, SW, W, NW, NE

		auto getEdgeNeighbourIndex = [&](size_t tileIndex, size_t edgeIndex) -> std::optional<size_t> {
			return getNeighbourIndex(tileIndex, edgeDirections[edgeIndex]);
		};

		auto getEdgeKey = [&](size_t tileIndex, size_t edgeIndex) -> std::pair<size_t, size_t> {
			auto neighbour = getEdgeNeighbourIndex(tileIndex, edgeIndex);
			size_t canonicalTileIndex = tileIndex;
			if (neighbour && *neighbour < canonicalTileIndex)
				canonicalTileIndex = *neighbour;

			size_t canonicalEdgeIndex = edgeIndex;
			if (neighbour) {
				const size_t otherTileIndex = (canonicalTileIndex == tileIndex) ? *neighbour : tileIndex;
				for (size_t ei = 0; ei < 6; ++ei) {
					if (auto n = getEdgeNeighbourIndex(canonicalTileIndex, ei); n && *n == otherTileIndex) {
						canonicalEdgeIndex = ei;
						break;
					}
				}
			}

			return {canonicalTileIndex, canonicalEdgeIndex};
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
		for (size_t tileIndex = 0; tileIndex < this->tiles.size(); ++tileIndex) {
			const auto& tile = this->tiles[tileIndex];
			fmt::println("[DEBUG].[populate] processing tile index {}", tileIndex);
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
				auto key = getVertexKey(tileIndex, vi);
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
				auto key = getEdgeKey(tileIndex, ei);
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
				auto vertex1Key = getVertexKey(tileIndex, ei);
				auto vertex2Key = getVertexKey(tileIndex, (ei + 1) % 6);
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
