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
				Player player;
				player.deserialize(playerJson);
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

	// Tutorial
	void GameState::initTutorial() {
		tutorialSteps.clear();

		tutorialSteps.push_back({.id = TutorialStepId::WELCOME,
								 .text =
									 "Welcome to Drengrfell.\n"
									 "You are a lone hero in a harsh land.\n"
									 "Lets start by looking around.\n"
									 "Press left mouse button to continue.",
								 .completed = false,
								 .screenPosition = std::nullopt,
								 .renderBox = true});

		tutorialSteps.push_back({.id = TutorialStepId::MOVE_CAMERA,
								 .text = "Use WASD to move the camera or simply move the cursor to the edges of the window.\nJust try it now!",
								 .completed = false,
								 .screenPosition = std::nullopt,
								 .renderBox = true});

		tutorialSteps.push_back({.id = TutorialStepId::CENTER_CAMERA,
								 .text = "Use 'Space' to center the camera onto the hero.\nThat way you can always find him no matter where you are!",
								 .completed = false,
								 .screenPosition = std::nullopt,
								 .renderBox = true});

		tutorialSteps.push_back({.id = TutorialStepId::ZOOM_CAMERA,
								 .text = "Use the mousewheel to zoom in/out.\nThis is also possible with +/-.",
								 .completed = false,
								 .screenPosition = std::nullopt,
								 .renderBox = true});

		tutorialSteps.push_back({.id = TutorialStepId::MOVE_HERO,
								 .text = "Use the right mouse button to click on a tile on the map to select and highlight it.\nAfter pressing the 'End Turn' button on the bottom right the hero will move there.\n"
										 "But beware, you might encounter a hazard.",
								 .completed = false,
								 .screenPosition = std::nullopt,
								 .renderBox = true});

		tutorialSteps.push_back({.id = TutorialStepId::OPEN_QUEST_MENU,
								 .text = "You can check your quests by pressing 'Q'! Your first quest will be\n to complete the tutorial.",
								 .completed = false,
								 .screenPosition = std::nullopt,
								 .renderBox = true});

		tutorialSteps.push_back({.id = TutorialStepId::BUILD_SETTLEMENT,
								 .text =
									 "Build your first settlement using the 'N' button.\n"
									 "Then you get the hover view.\n"
									 "Here click any free tile close to your hero to build the settlement.\n"
									 "Settlements generate resources from nearby tiles each round.",
								 .completed = false,
								 .screenPosition = std::nullopt,
								 .renderBox = true});

		tutorialSteps.push_back({.id = TutorialStepId::BUILD_ROAD,
								 .text = "Build a road to expand using 'B' Button to create the hover view.\nThen select any free edge close to your hero to build the road.",
								 .completed = false,
								 .screenPosition = std::nullopt,
								 .renderBox = true});

		tutorialSteps.push_back({.id = TutorialStepId::OPEN_TRADE_MENU,
								 .text = "Use 'T' to open the trade menu and trade your ressources.",
								 .completed = false,
								 .screenPosition = std::nullopt,
								 .renderBox = true});
				
		tutorialSteps.push_back({.id = TutorialStepId::OPEN_KEYBINDS_MENU,
								 .text = "You can see all the possible keybinds by pressing the 'K' button.",
								 .completed = false,
								 .screenPosition = std::nullopt,
								 .renderBox = true});

		tutorialSteps.push_back({.id = TutorialStepId::END,
								 .text = "Tutorial completed! \nPress left mouse button to exit the tutorial.",
								 .completed = false,
								 .screenPosition = std::nullopt,
								 .renderBox = true});
	}

	void GameState::resetTutorial() {
		currentTutorialStep = 0;
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

	bool GameState::isGameOver() const {
		const size_t MAX_ROUNDS = 50; // Or whatever limit you want
		return this->roundNumber >= MAX_ROUNDS;
	}


} // namespace df
