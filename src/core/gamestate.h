#pragma once

#include <filesystem>
#include <memory>
#include <vector>
#include "registry.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "graph.h"
#include "player.h"
#include "road.h"
#include "settlement.h"
#include "types.h"





namespace df {


    // game state information storage -> used for saving/loading and (potentially)
    // multiplayer syncing
    class GameState {
    public:
        GameState() = default;
        GameState(Registry* reg) : registry(reg) {};


        Graph getMap() { return this->map; }
        void setMap(Graph newMap) { this->map = newMap; }


        // players
        size_t getPlayerCount() const { return this->players.size(); }
        Player *getPlayer(size_t playerId);
        std::vector<Player> &getPlayers() { return this->players; }
        void addPlayer(const Player &player) { this->players.push_back(player); }
        void clearPlayers() { this->players.clear(); }


        // settlements
        std::vector<Settlement*> getSettlements();
        void addSettlement(const Settlement& settlementData);
        void clearSettlements() { registry->settlements.clear(); }


        // roads
        std::vector<Road*> getRoads();
        void addRoad(const Road& roadData);
        void clearRoads() { registry->roads.clear(); }



        // turns
        size_t getCurrentPlayerId() const { return this->currentPlayerId; }
        void setCurrentPlayerId(size_t playerId) { this->currentPlayerId = playerId; }

        size_t getTurnCount() const { return this->turnCount; }
        void setTurnCount(size_t count) { this->turnCount = count; }

        size_t getRoundNumber() const { return this->roundNumber; }
        void setRoundNumber(size_t round) { this->roundNumber = round; }

        types::GamePhase getPhase() const { return this->phase; }
        void setPhase(types::GamePhase newPhase) { this->phase = newPhase; }


        // persistence
        json serialize() const;
        void deserialize(const json &j);
        void save(const std::filesystem::path &filepath) const;
        void load(const std::filesystem::path &filepath);



    private:
        // TODO: discuss ownership model for game...
        Graph map;

        std::vector<Player> players;

        // turns
        size_t currentPlayerId = 0;
        size_t turnCount = 0;
        size_t roundNumber = 0;
        types::GamePhase phase = types::GamePhase::SETUP;
        Registry* registry = nullptr;;
    };

}


