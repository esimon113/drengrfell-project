#include "gamestate.h"

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


    /**
     * Serialize the game state and return it as a json object.
     */
    json GameState::serialize() const {
        json j;

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
            settlementsJson.push_back(settlement->serialize());
        }
        j["settlements"] = settlementsJson;

        // roads
        json roadsJson = json::array();
        for (const auto& road : this->roads) {
            roadsJson.push_back(road->serialize());
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
        this->settlements.clear();
        this->roads.clear();


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
                
                Player player(playerId);
                player.deserialize(playerJson);
                this->players.push_back(player);
            }
        }

        // settlements
        if (j.contains("settlements") && j["settlements"].is_array()) {
            for (const auto& settlementJson : j["settlements"]) {
                auto settlement = std::make_unique<Settlement>();
                settlement->deserialize(settlementJson);
                this->settlements.push_back(std::move(settlement));
            }
        }

        // roads
        if (j.contains("roads") && j["roads"].is_array()) {
            for (const auto& roadJson : j["roads"]) {
                auto road = std::make_unique<Road>();
                road->deserialize(roadJson);
                this->roads.push_back(std::move(road));
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

} // namespace df
