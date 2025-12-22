#include "gamestate.h"
#include "utils/worldNodeMapper.h"
#include <fstream>
#include <stdexcept>





namespace df {

    /**
     * Returns a pointer to the player with the given id.
     * Returns nullptr if the player id is not found.
     */
    Player *GameState::getPlayer(size_t playerId) {
        if (playerId >= this->players.size()) { // check for valid id
            // maybe throw an error here?
            return nullptr;
        }
        return &this->players[playerId];
    }


    const Player *GameState::getPlayer(size_t playerId) const {
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
    void GameState::deserialize(const json &j) {
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

        // turns
        if (j.contains("currentPlayerId")) { this->setCurrentPlayerId(j["currentPlayerId"].get<size_t>()); }
        if (j.contains("turnCount")) { this->setTurnCount(j["turnCount"].get<size_t>()); }
        if (j.contains("roundNumber")) { this->setRoundNumber(j["roundNumber"].get<size_t>()); }
        if (j.contains("phase")) { this->setPhase(static_cast<types::GamePhase>(j["phase"].get<int>())); }
    }


    /**
     * Serialize the game state and store in the passed filepaht.
     */
    void GameState::save(const std::filesystem::path &filepath) const {
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
    void GameState::load(const std::filesystem::path &filepath) {
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
        if (!settlement || !registry) { return; }

        // Also add to ECS registry for rendering/systems
        Entity e;
        Settlement& s = registry->settlements.emplace(e);
        s = *settlement; // Copy data to ECS

        // Add position and scale components for rendering
        const Graph& map = this->map;
        glm::vec2 worldPosition = WorldNodeMapper::getWorldPositionForVertex(settlement->getVertexId(), map);
        registry->positions.emplace(e) = worldPosition;
        registry->scales.emplace(e) = glm::vec2(0.5f, 0.5f); // Scale to match hexagon size -> 1/2 hex radius

        settlements.push_back(settlement);
    }


    // roads
    void GameState::addRoad(std::shared_ptr<Road> road) {
        if (!road || !registry) { return; }

        // Also add to ECS registry for rendering/systems
        Entity e;
        Road& r = registry->roads.emplace(e);
        r = *road; // Copy data to ECS

        // Add position and scale components for rendering
        const Graph& map = this->map;
        glm::vec2 worldPosition = WorldNodeMapper::getWorldPositionForEdge(road->getEdgeId(), map);
        registry->positions.emplace(e) = worldPosition;
        registry->scales.emplace(e) = glm::vec2(1.0f, 1.0f);

        roads.push_back(road);
    }

    std::vector<std::shared_ptr<Road>> GameState::getRoads() {
        return roads;
    }

    // Tutorial
    void GameState::initTutorial() {
        tutorialSteps.clear();

        tutorialSteps.push_back({
            .id = TutorialStepId::WELCOME,
            .text =
                "Welcome to Drengrfell.\n"
                "You are a lone hero in a harsh land.\n"
                "Lets start by looking around.\n"
                "Press left mouse button to continue.",
            .completed = false,
            .screenPosition = std::nullopt,
            .renderBox = true
            });

        tutorialSteps.push_back({
            .id = TutorialStepId::MOVE_CAMERA,
            .text = "Use WASD to move the camera. Just try it now!",
            .completed = false,
            .screenPosition = std::nullopt,
            .renderBox = true
            });

        tutorialSteps.push_back({
            .id = TutorialStepId::ZOOM_CAMERA,
            .text = "Use the mousewheel to zoom in/out.\nThis is also possible with +/-.",
            .completed = false,
            .screenPosition = std::nullopt,
            .renderBox = true
            });

        tutorialSteps.push_back({
            .id = TutorialStepId::BUILD_SETTLEMENT,
            .text =
                "Build your first settlement using the n Button.\n"
                "Then you get the hover view.\n"
                "Here click any free tile close to your hero to build the settlement.\n"
                "Settlements generate resources from nearby tiles each round.",
            .completed = false,
            .screenPosition = std::nullopt,
            .renderBox = true
            });

        tutorialSteps.push_back({
            .id = TutorialStepId::BUILD_ROAD,
            .text = "Build a road to expand using b Button to create the hover view.\nThen select any free edge close to your hero to build the road.",
            .completed = false,
            .screenPosition = std::nullopt,
            .renderBox = true
            });

        tutorialSteps.push_back({
            .id = TutorialStepId::END,
            .text = "Tutorial completed! \nPress left mouse button to exit the tutorial.",
            .completed = false,
            .screenPosition = std::nullopt,
            .renderBox = true
            });
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
        const size_t MAX_ROUNDS = 20; // Or whatever limit you want
        return this->roundNumber >= MAX_ROUNDS;
    }


} // namespace df
