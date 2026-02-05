#include "gamestate.h"
#include "utils/worldNodeMapper.h"
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

		fmt::println("Serializing Game state");
		fmt::println("---------");
		fmt::println("Map: {}", this->map.serialize().dump());
		fmt::println("---------");
		fmt::println("Players: {}", this->players.size());
		fmt::println("---------");
		fmt::println("Settlements: {}", this->settlements.size());
		fmt::println("---------");
		fmt::println("Roads: {}", this->roads.size());
		fmt::println("---------");
		fmt::println("Current player id: {}", this->currentPlayerId);
		fmt::println("---------");

		// map
		j["map"] = this->map.serialize();

		// players
		json playersJson = json::array();
		for (const auto& player : this->players) { // TODO
			// playersJson.push_back(player.serialize());
			playersJson.push_back(player.getId());
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

		return j;
	}


	/**
	 * Deserializes the game state from the provided json object. This can be used to load a saved game state from a file.
	 */
	void GameState::deserialize(const json& j) {
		// clear current state
		this->players.clear();

		// map
		if (j.contains("map") && j["map"].is_object() && !j["map"].empty()) {
			std::string mapData = j["map"].dump();
			this->map.deserialize(mapData);
		}

		// players
		if (j.contains("players") && j["players"].is_array()) {
			for (const auto& playerJson : j["players"]) {
				size_t playerId = 0;
				if (playerJson.contains("id") && playerJson["id"].is_number()) {
					playerId = playerJson["id"].get<size_t>();
				}

				Player player(playerId); // TODO
				// player.deserialize(playerJson);
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
		if (!settlement || !registry) {
			return;
		}

		// Also add to ECS registry for rendering/systems
		Entity e;
		Settlement& s = registry->settlements.emplace(e);
		s = *settlement; // Copy data to ECS

		// Add position and scale components for rendering
		registry->positions.emplace(e) = WorldNodeMapper::getWorldPositionForVertex(settlement->getVertexId(), this->map);
		registry->scales.emplace(e) = glm::vec2(0.45f, 0.45f); // Scale to match hexagon size -> 1/2 hex radius

		settlements.push_back(settlement);
	}


	// roads
	void GameState::addRoad(std::shared_ptr<Road> road) {
		if (!road || !registry) {
			return;
		}

		// Also add to ECS registry for rendering/systems
		Entity e;
		Road& r = registry->roads.emplace(e);
		r = *road; // Copy data to ECS

		// Add position and scale components for rendering
		registry->positions.emplace(e) = WorldNodeMapper::getWorldPositionForEdge(road->getEdgeId(), this->map);
		registry->scales.emplace(e) = glm::vec2(1.0f, 1.0f);

		// edge index is required for selecting the correcxt texture
		int edgeIndex = this->map.getEdgeIndex(road->getEdgeId());
		registry->roadEdgeIndices.emplace(e) = edgeIndex;

		roads.push_back(road);
	}

	std::vector<std::shared_ptr<Road>> GameState::getRoads() {
		return roads;
	}

std::vector<std::shared_ptr<ProductivityBuilding>> GameState::getProductivityBuildings() {
	return productivityBuildings;
}

void GameState::addProductivityBuilding(std::shared_ptr<ProductivityBuilding> building) {
	if (!building || !registry) {
		return;
	}

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
		if (currentTutorialStep < tutorialSteps.size()) {
			tutorialSteps[currentTutorialStep].completed = true;
			currentTutorialStep++;
		}
	}

	bool GameState::isTutorialActive() const {
		return currentTutorialStep < tutorialSteps.size();
	}

	// returns a vector<glm::vec3> with the corresponding colors the hud should use for the resources
	std::vector<glm::vec3> GameState::computeHudResourceColor(std::string mode) {
		// order: forest, mountain, clay, grass (wool), field
		std::vector<glm::vec3> colors = {{1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}};
		std::map<types::TileType, int> playerResources = getPlayer(getCurrentPlayerId())->getResources();

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

	bool GameState::isGameOver() const {
		const int WINNING_POINTS = 20; 

		for (const auto& player : this->players) {
			if (player.getHeroPoints() >= WINNING_POINTS) {
				fmt::println("[GameState] Player {} has reached {} points! Game Over.", 
							player.getId(), player.getHeroPoints());
				return true;
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

			if (castleCount >= 3) {
				fmt::println("[GameState] Player {} built 3 Castles!", player.getId());
				return true;
			}
		}
		return false;
	}


} // namespace df
