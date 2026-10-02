#include "gamestate.h"
#include "utils/worldNodeMapper.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>





namespace df {

	/**
	 * Returns a pointer to the player with the given id.
	 * Returns nullptr if the player id is not found.
	 */
	Player* GameState::getPlayer(size_t playerId) {
		if (playerId >= this->players.size()) { // check for valid id
			// maybe throw an error here?
			return nullptr;
		}
		return &this->players[playerId];
	}


	const Player* GameState::getPlayer(size_t playerId) const {
		if (playerId >= this->players.size()) {
			return nullptr;
		}
		return &this->players[playerId];
	}


	/**
	 * Serialize the game state and return it as a json object.
	 */
	json GameState::serialize() const {
		json j;

		j["world"] = this->worldConfig.serialize();

		json playersJson = json::array();
		for (const auto& player : this->players) {
			playersJson.push_back(player.serialize());
		}
		j["players"] = playersJson;

		// settlements
		json settlementsJson = json::array();
		for (const auto& settlement : this->settlements) {
			if (settlement) {
				settlementsJson.push_back(settlement->serialize());
			}
		}
		j["settlements"] = settlementsJson;

		// roads
		json roadsJson = json::array();
		for (const auto& road : this->roads) {
			if (road) {
				roadsJson.push_back(road->serialize());
			}
		}
		j["roads"] = roadsJson;

	// productivity buildings
	json productivityBuildingsJson = json::array();
	for (const auto& building : this->productivityBuildings) {
		if (building) {
			productivityBuildingsJson.push_back(building->serialize());
		}
	}
	j["productivityBuildings"] = productivityBuildingsJson;

		// turns
		j["currentPlayerId"] = this->currentPlayerId;
		j["turnCount"] = this->turnCount;
		j["roundNumber"] = this->roundNumber;
		j["phase"] = static_cast<int>(this->phase);
		j["currentTutorialStep"] = this->currentTutorialStep;
		j["weather"] = static_cast<int>(this->weather);
		j["weatherIntensity"] = this->weatherIntensity;
		const auto winnerId = computeWinnerId();
		j["winnerId"] = winnerId ? json(*winnerId) : json(nullptr);

		return j;
	}


	/**
	 * Deserializes the game state from the provided json object. This can be used to load a saved game state from a file.
	 */
	void GameState::deserialize(const json& j) {
		// clear current state
		this->players.clear();

		if (j.contains("world") && j["world"].is_object()) {
			this->worldConfig = WorldGeneratorConfig::deserialize(j["world"]);
			this->map.regenerate(this->worldConfig);
		} else if (j.contains("map") && j["map"].is_object() && !j["map"].empty()) {
			std::string mapData = j["map"].dump();
			this->map.deserialize(mapData);
		}

		if (j.contains("players") && j["players"].is_array()) {
			for (const auto& playerJson : j["players"]) {
				Player player;
				if (playerJson.is_object()) {
					player.deserialize(playerJson);
				} else if (playerJson.is_number()) {
					player = Player(playerJson.get<size_t>());
				}
				this->players.push_back(player);
			}
		}

		// settlements
		if (j.contains("settlements") && j["settlements"].is_array()) {
			for (const auto& settlementJson : j["settlements"]) {
				auto settlement = std::make_shared<Settlement>();
				settlement->deserialize(settlementJson);
				this->addSettlement(settlement);
			}
		}

		// roads
		if (j.contains("roads") && j["roads"].is_array()) {
			for (const auto& roadJson : j["roads"]) {
				auto road = std::make_shared<Road>();
				road->deserialize(roadJson);
				this->addRoad(road);
			}
		}

	// productivity buildings
	if (j.contains("productivityBuildings") && j["productivityBuildings"].is_array()) {
		for (const auto& buildingJson : j["productivityBuildings"]) {
			auto building = std::make_shared<ProductivityBuilding>();
			building->deserialize(buildingJson);
			this->addProductivityBuilding(building);
		}
	}

		// turns
		if (j.contains("currentPlayerId")) {
			this->setCurrentPlayerId(j["currentPlayerId"].get<size_t>());
		}
		if (j.contains("turnCount")) {
			this->setTurnCount(j["turnCount"].get<size_t>());
		}
		if (j.contains("roundNumber")) {
			this->setRoundNumber(j["roundNumber"].get<size_t>());
		}
		if (j.contains("phase")) {
			this->setPhase(static_cast<types::GamePhase>(j["phase"].get<int>()));
		}
	}

	void GameState::applyAuthoritativeSnapshot(const json& j) {
		WorldGeneratorConfig incoming;
		const bool haveWorld = j.contains("world") && j["world"].is_object();
		if (haveWorld) {
			incoming = WorldGeneratorConfig::deserialize(j["world"]);
		}

		const bool needsMap = this->map.getTileCount() == 0;
		const bool mapChanged = !haveWorld || needsMap || incoming.seed != this->worldConfig.seed ||
			incoming.columns != this->worldConfig.columns || incoming.rows != this->worldConfig.rows;
		if (haveWorld && (needsMap || mapChanged)) {
			this->worldConfig = incoming;
			this->map.regenerate(this->worldConfig);
		} else {
			for (const auto& vertex : this->map.getVertices()) {
				if (vertex) {
					vertex->setSettlementId(std::nullopt);
				}
			}
			for (const auto& edge : this->map.getEdges()) {
				if (edge) {
					edge->setRoadId(std::nullopt);
				}
			}
			for (const auto& tile : this->map.getTiles()) {
				if (tile) {
					tile->setBuildingId(std::nullopt);
					tile->setVisibleForPlayers({});
				}
			}
		}

		this->clearSettlements();
		this->clearRoads();
		this->clearProductivityBuildings();
		this->players.clear();

		if (j.contains("players") && j["players"].is_array()) {
			for (const auto& playerJson : j["players"]) {
				Player player;
				if (playerJson.is_object()) {
					player.deserialize(playerJson);
				} else if (playerJson.is_number()) {
					player = Player(playerJson.get<size_t>());
				}
				this->players.push_back(player);
			}
		}

		if (j.contains("settlements") && j["settlements"].is_array()) {
			for (const auto& settlementJson : j["settlements"]) {
				auto settlement = std::make_shared<Settlement>();
				settlement->deserialize(settlementJson);
				if (VertexHandle vertex = this->map.findVertexById(settlement->getVertexId())) {
					vertex->setSettlementId(settlement->getId());
				}
				this->addSettlement(settlement);
			}
		}

		if (j.contains("roads") && j["roads"].is_array()) {
			for (const auto& roadJson : j["roads"]) {
				auto road = std::make_shared<Road>();
				road->deserialize(roadJson);
				if (EdgeHandle edge = this->map.findEdgeById(road->getEdgeId())) {
					edge->setRoadId(road->getId());
				}
				this->addRoad(road);
			}
		}

		if (j.contains("productivityBuildings") && j["productivityBuildings"].is_array()) {
			for (const auto& buildingJson : j["productivityBuildings"]) {
				auto building = std::make_shared<ProductivityBuilding>();
				building->deserialize(buildingJson);
				if (building->getTileId() < this->map.getTileCount()) {
					if (TileHandle tile = this->map.getTile(building->getTileId())) {
						tile->setBuildingId(building->getPlayerId());
					}
				}
				this->addProductivityBuilding(building);
			}
		}

		if (j.contains("currentPlayerId")) {
			this->currentPlayerId = j["currentPlayerId"].get<size_t>();
		}
		if (j.contains("turnCount")) {
			this->turnCount = j["turnCount"].get<size_t>();
		}
		if (j.contains("roundNumber")) {
			this->roundNumber = j["roundNumber"].get<size_t>();
		}
		if (j.contains("phase")) {
			this->phase = static_cast<types::GamePhase>(j["phase"].get<int>());
		}
		if (j.contains("winnerId") && !j["winnerId"].is_null()) {
			this->authoritativeWinnerId = j["winnerId"].get<size_t>();
		} else {
			this->authoritativeWinnerId = std::nullopt;
		}

		if (this->tutorialSteps.empty()) {
			this->initTutorial();
		}
		if (j.contains("currentTutorialStep")) {
			this->currentTutorialStep = j["currentTutorialStep"].get<size_t>();
			for (size_t i = 0; i < this->tutorialSteps.size(); ++i) {
				this->tutorialSteps[i].completed = i < this->currentTutorialStep;
			}
		}
		this->tutorialReportSentFor = static_cast<size_t>(-1);
		this->authoritativeMap = true;

		if (j.contains("weather")) {
			this->weather = static_cast<types::WeatherType>(j["weather"].get<int>());
		}
		if (j.contains("weatherIntensity")) {
			this->weatherIntensity = j["weatherIntensity"].get<float>();
		}
		for (const auto& tile : this->map.getTiles()) {
			if (tile) {
				tile->updateEffect(this->weather);
			}
		}

		for (const Player& player : this->players) {
			for (size_t tileId : player.getExploredTileIds()) {
				if (tileId < this->map.getTileCount()) {
					if (TileHandle tile = this->map.getTile(tileId)) {
						tile->addVisibleForPlayers(player.getId());
					}
				}
			}
		}

		if (Player* viewer = getPlayer(viewerPlayerId)) {
			currentTutorialStep = std::min(viewer->getTutorialStep(), tutorialSteps.empty() ? size_t{0} : tutorialSteps.size());
			for (size_t i = 0; i < currentTutorialStep && i < tutorialSteps.size(); ++i) {
				tutorialSteps[i].completed = true;
			}
		}

		this->map.setRenderUpdateRequested(true);
	}


	/**
	 * Serialize the game state and store in the passed filepaht.
	 */
	void GameState::save(const std::filesystem::path& filepath) const {
		std::ofstream file(filepath);

		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file for writing: " + filepath.string());
		}

		file << this->serialize().dump(4);
		file.close();
	}


	/**
	 * Load the game state from the passed filepath and store it in the gamestate object.
	 */
	void GameState::load(const std::filesystem::path& filepath) {
		std::ifstream file(filepath);

		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file for reading: " + filepath.string());
		}

		std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();

		json j = json::parse(data);
		this->deserialize(j);
	}

	// settlements
	std::vector<std::shared_ptr<Settlement>> GameState::getSettlements() {
		return settlements;
	}

	void GameState::addSettlement(std::shared_ptr<Settlement> settlement) {
		if (!settlement) {
			return;
		}

		if (registry) {
			Entity e;
			Settlement& s = registry->settlements.emplace(e);
			s = *settlement;
			registry->positions.emplace(e) = WorldNodeMapper::getWorldPositionForVertex(settlement->getVertexId(), this->map);
			registry->scales.emplace(e) = glm::vec2(0.45f, 0.45f);
		}

		settlements.push_back(settlement);
	}


	// roads
	void GameState::addRoad(std::shared_ptr<Road> road) {
		if (!road) {
			return;
		}

		if (registry) {
			Entity e;
			Road& r = registry->roads.emplace(e);
			r = *road;
			registry->positions.emplace(e) = WorldNodeMapper::getWorldPositionForEdge(road->getEdgeId(), this->map);
			registry->scales.emplace(e) = glm::vec2(1.0f, 1.0f);
			int edgeIndex = this->map.getEdgeIndex(road->getEdgeId());
			registry->roadEdgeIndices.emplace(e) = edgeIndex;
		}

		roads.push_back(road);
	}

	std::vector<std::shared_ptr<Road>> GameState::getRoads() {
		return roads;
	}

std::vector<std::shared_ptr<ProductivityBuilding>> GameState::getProductivityBuildings() {
	return productivityBuildings;
}

void GameState::addProductivityBuilding(std::shared_ptr<ProductivityBuilding> building) {
	if (!building) {
		return;
	}

	if (registry) {
		Entity e;
		ProductivityBuilding& pb = registry->productivityBuildings.emplace(e);
		pb = *building;

		const uint32_t columns = map.getMapWidth();
		const size_t tileId = building->getTileId();
		uint32_t row = static_cast<uint32_t>(tileId / columns);
		uint32_t col = static_cast<uint32_t>(tileId % columns);
		glm::vec2 tileCenterPos = WorldNodeMapper::getTilePosition(row, col);

		registry->positions.emplace(e) = tileCenterPos;
		registry->scales.emplace(e) = glm::vec2(0.4f, 0.4f);
	}

	productivityBuildings.push_back(building);
}


	// TODO: balance costs + make costs scale with total available resources
	const std::vector<int>& GameState::getCurrentRoadCost() const {
		return this->roadCosts;
	}


	const std::vector<int>& GameState::getCurrentSettlementCost() const {
		return this->settlementCosts;
	}

	// Tutorial init (was moved to core/tutorial.cpp)
	void GameState::initTutorial() {
		tutorialSteps.clear();
		tutorialSteps = createDefaultTutorial();
		currentTutorialStep = 0;
	}

	void GameState::resetTutorial() {
		initTutorial();
	}

	TutorialStep* GameState::getCurrentTutorialStep() {
		if (currentTutorialStep >= tutorialSteps.size()) {
			return nullptr;
		}
		return &tutorialSteps[currentTutorialStep];
	}

	void GameState::completeCurrentTutorialStep() {
		if (currentTutorialStep >= tutorialSteps.size()) {
			return;
		}
		if (tutorialReporter) {
			if (tutorialReportSentFor == currentTutorialStep) {
				return;
			}
			tutorialReportSentFor = currentTutorialStep;
			tutorialReporter(tutorialSteps[currentTutorialStep].id);
			return;
		}
		tutorialSteps[currentTutorialStep].completed = true;
		currentTutorialStep++;
	}

	void GameState::completeTutorialStep(TutorialStepId id) {
		if (tutorialSteps.empty()) {
			initTutorial();
		}
		const TutorialStep* step = getCurrentTutorialStep();
		if (step && step->id == id) {
			completeCurrentTutorialStep();
		}
	}

	bool GameState::completeTutorialStepFor(size_t playerId, TutorialStepId id) {
		if (tutorialSteps.empty()) {
			initTutorial();
		}
		Player* player = getPlayer(playerId);
		if (!player) {
			return false;
		}
		const size_t index = player->getTutorialStep();
		if (index >= tutorialSteps.size() || tutorialSteps[index].id != id) {
			return false;
		}
		player->setTutorialStep(index + 1);
		return true;
	}

	bool GameState::isTutorialActive() const {
		return currentTutorialStep < tutorialSteps.size();
	}

	// returns a vector<glm::vec3> with the corresponding colors the hud should use for the resources
	std::vector<glm::vec3> GameState::computeHudResourceColor(std::string mode) {
		// order: forest, mountain, clay, grass (wool), field
		std::vector<glm::vec3> colors = {{1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}};
		std::map<types::TileType, int> playerResources = getPlayer(getViewerPlayerId())->getResources();

		if (mode == "settlement") {
			// settlements
			const auto settlementCost = getCurrentSettlementCost();
			// wood
			if (settlementCost[2] > 0 && playerResources[types::TileType::FOREST] >= settlementCost[2])
				colors[0] = {0.f, 1.f, 0.f};
			else if (settlementCost[2] > 0)
				colors[0] = {1.f, 0.f, 0.f};
			// grass
			if (settlementCost[3] > 0 && playerResources[types::TileType::GRASS] >= settlementCost[3])
				colors[3] = {0.f, 1.f, 0.f};
			else if (settlementCost[3] > 0)
				colors[3] = {1.f, 0.f, 0.f};
			// stone
			if (settlementCost[4] > 0 && playerResources[types::TileType::MOUNTAIN] >= settlementCost[4])
				colors[1] = {0.f, 1.f, 0.f};
			else if (settlementCost[4] > 0)
				colors[1] = {1.f, 0.f, 0.f};
			// grain
			if (settlementCost[5] > 0 && playerResources[types::TileType::FIELD] >= settlementCost[5])
				colors[4] = {0.f, 1.f, 0.f};
			else if (settlementCost[5] > 0)
				colors[4] = {1.f, 0.f, 0.f};
			// clay
			if (settlementCost[6] > 0 && playerResources[types::TileType::CLAY] >= settlementCost[6])
				colors[2] = {0.f, 1.f, 0.f};
			else if (settlementCost[6] > 0)
				colors[2] = {1.f, 0.f, 0.f};
		}
		else {
			// roads
			const auto roadCost = getCurrentRoadCost();
			// wood
			if (roadCost[2] > 0 && playerResources[types::TileType::FOREST] >= roadCost[2])
				colors[0] = {0.f, 1.f, 0.f};
			else if (roadCost[2] > 0)
				colors[0] = {1.f, 0.f, 0.f};
			// grass
			if (roadCost[3] > 0 && playerResources[types::TileType::GRASS] >= roadCost[3])
				colors[3] = {0.f, 1.f, 0.f};
			else if (roadCost[3] > 0)
				colors[3] = {1.f, 0.f, 0.f};
			// stone
			if (roadCost[4] > 0 && playerResources[types::TileType::MOUNTAIN] >= roadCost[4])
				colors[1] = {0.f, 1.f, 0.f};
			else if (roadCost[4] > 0)
				colors[1] = {1.f, 0.f, 0.f};
			// grain
			if (roadCost[5] > 0 && playerResources[types::TileType::FIELD] >= roadCost[5])
				colors[4] = {0.f, 1.f, 0.f};
			else if (roadCost[5] > 0)
				colors[4] = {1.f, 0.f, 0.f};
			// clay
			if (roadCost[6] > 0 && playerResources[types::TileType::CLAY] >= roadCost[6])
				colors[2] = {0.f, 1.f, 0.f};
			else if (roadCost[6] > 0)
				colors[2] = {1.f, 0.f, 0.f};
			}
		return colors;
	}

	std::optional<size_t> GameState::computeWinnerId() const {
		for (const auto& player : this->players) {
			if (player.getHeroPoints() >= WINNING_POINTS) {
				return player.getId();
			}

			int castleCount = 0;
			for (size_t sId : player.getSettlementIds()) {
				for (const auto& settlement : this->settlements) {
					if (settlement && settlement->getId() == sId) {
						if (settlement->getSettlementType() == types::SettlementType::CASTLE) {
							castleCount++;
						}
						break;
					}
				}
			}

			if (castleCount >= WINNING_CASTLES) {
				return player.getId();
			}
		}
		return std::nullopt;
	}

	std::optional<size_t> GameState::getWinnerId() const {
		if (authoritativeMap) {
			return authoritativeWinnerId;
		}
		return computeWinnerId();
	}

	bool GameState::isGameOver() const {
		const auto winnerId = getWinnerId();
		if (!winnerId) {
			return false;
		}

		const auto& player = this->players[*winnerId];
		if (player.getHeroPoints() >= WINNING_POINTS) {
			fmt::println("[GameState] Player {} has reached {} points! Game Over.",
						player.getId(), player.getHeroPoints());
		} else {
			fmt::println("[GameState] Player {} built 3 Castles!", player.getId());
		}
		return true;
	}

	bool GameState::isTileVisibleTo(size_t playerId, size_t tileId) const {
		const Player* player = getPlayer(playerId);
		if (!player) {
			return false;
		}
		return player->isTileExplored(tileId);
	}

	bool GameState::isVertexVisibleTo(size_t playerId, size_t vertexId) const {
		VertexHandle vertex = map.findVertexById(vertexId);
		if (!vertex) {
			return false;
		}
		const auto tiles = map.getVertexTiles(vertex);
		if (!tiles) {
			return false;
		}
		for (const auto& tile : *tiles) {
			if (tile && isTileVisibleTo(playerId, tile->getId())) {
				return true;
			}
		}
		return false;
	}

	bool GameState::isEdgeVisibleTo(size_t playerId, size_t edgeId) const {
		EdgeHandle edge = map.findEdgeById(edgeId);
		if (!edge) {
			return false;
		}
		const auto vertices = map.getEdgeVertices(edge);
		if (!vertices) {
			return false;
		}
		for (const auto& vertex : *vertices) {
			if (vertex && isVertexVisibleTo(playerId, vertex->getId())) {
				return true;
			}
		}
		return false;
	}

	void GameState::syncSettlementType(size_t settlementId, types::SettlementType type) {
		if (!registry) {
			return;
		}
		for (Entity e : registry->settlements.entities) {
			if (!registry->settlements.has(e)) {
				continue;
			}
			Settlement& registrySettlement = registry->settlements.get(e);
			if (registrySettlement.getId() == settlementId) {
				registrySettlement.setSettlementType(type);
				break;
			}
		}
	}

	json GameState::serializeFor(size_t viewerPlayerId) const {
		json j = serialize();
		if (!j.contains("players") || !j["players"].is_array()) {
			return j;
		}

		json filteredPlayers = json::array();
		for (auto playerJson : j["players"]) {
			if (!playerJson.is_object()) {
				continue;
			}
			const size_t id = playerJson.value("playerId", static_cast<size_t>(0));
			if (id == viewerPlayerId) {
				filteredPlayers.push_back(std::move(playerJson));
				continue;
			}

			playerJson.erase("resources");
			playerJson.erase("exploredTileIds");
			if (playerJson.contains("hero") && playerJson["hero"].contains("tileID")) {
				const size_t heroTile = playerJson["hero"]["tileID"].get<size_t>();
				if (!isTileVisibleTo(viewerPlayerId, heroTile)) {
					playerJson.erase("hero");
				}
			}
			filteredPlayers.push_back(std::move(playerJson));
		}
		j["players"] = std::move(filteredPlayers);

		json filteredSettlements = json::array();
		if (j.contains("settlements") && j["settlements"].is_array()) {
			for (const auto& settlementJson : j["settlements"]) {
				if (!settlementJson.contains("vertexId")) {
					continue;
				}
				const size_t ownerId = settlementJson.value("playerId", static_cast<size_t>(-1));
				if (ownerId == viewerPlayerId || isVertexVisibleTo(viewerPlayerId, settlementJson["vertexId"].get<size_t>())) {
					filteredSettlements.push_back(settlementJson);
				}
			}
		}
		j["settlements"] = std::move(filteredSettlements);

		json filteredRoads = json::array();
		if (j.contains("roads") && j["roads"].is_array()) {
			for (const auto& roadJson : j["roads"]) {
				if (!roadJson.contains("edgeId")) {
					continue;
				}
				const size_t ownerId = roadJson.value("playerId", static_cast<size_t>(-1));
				if (ownerId == viewerPlayerId || isEdgeVisibleTo(viewerPlayerId, roadJson["edgeId"].get<size_t>())) {
					filteredRoads.push_back(roadJson);
				}
			}
		}
		j["roads"] = std::move(filteredRoads);

		json filteredBuildings = json::array();
		if (j.contains("productivityBuildings") && j["productivityBuildings"].is_array()) {
			for (const auto& buildingJson : j["productivityBuildings"]) {
				if (!buildingJson.contains("tileId")) {
					continue;
				}
				const size_t ownerId = buildingJson.value("playerId", static_cast<size_t>(-1));
				if (ownerId == viewerPlayerId || isTileVisibleTo(viewerPlayerId, buildingJson["tileId"].get<size_t>())) {
					filteredBuildings.push_back(buildingJson);
				}
			}
		}
		j["productivityBuildings"] = std::move(filteredBuildings);

		return j;
	}


} // namespace df
