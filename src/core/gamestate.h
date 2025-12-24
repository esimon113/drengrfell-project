#pragma once

#include <filesystem>
#include <vector>
#include <memory>
#include "registry.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "graph.h"
#include "player.h"
#include "road.h"
#include "settlement.h"
#include "types.h"
#include "tutorial.h"





namespace df {


    // game state information storage -> used for saving/loading and (potentially)
    // multiplayer syncing
    class GameState {
    public:
        GameState() = default;
        GameState(Registry* reg) : registry(reg) {};


        Graph& getMap() { return this->map; }
        const Graph& getMap() const { return this->map; }
        void setMap(Graph newMap) { this->map = std::move(newMap); }


        // players
        size_t getPlayerCount() const { return this->players.size(); }
        Player *getPlayer(size_t playerId);
        const Player *getPlayer(size_t playerId) const;
        std::vector<Player> &getPlayers() { return this->players; }
        const std::vector<Player> &getPlayers() const { return this->players; }
        void addPlayer(const Player &player) { this->players.push_back(player); }
        void clearPlayers() { this->players.clear(); }


        // settlements
        std::vector<std::shared_ptr<Settlement>> getSettlements();
        void addSettlement(std::shared_ptr<Settlement> settlement);
        void clearSettlements() { 
            settlements.clear(); 
            registry->settlements.clear(); 
        }


        // roads
        std::vector<std::shared_ptr<Road>> getRoads();
        void addRoad(std::shared_ptr<Road> road);
        void clearRoads() { 
            roads.clear(); 
            registry->roads.clear(); 
        }


        // turns
        size_t getCurrentPlayerId() const { return this->currentPlayerId; }
        void setCurrentPlayerId(size_t playerId) { this->currentPlayerId = playerId; }

        size_t getTurnCount() const { return this->turnCount; }
        void setTurnCount(size_t count) { this->turnCount = count; }

        size_t getRoundNumber() const { return this->roundNumber; }
        void setRoundNumber(size_t round) { this->roundNumber = round; }

        types::GamePhase getPhase() const { return this->phase; }
        void setPhase(types::GamePhase newPhase) { this->phase = newPhase; fmt::println("Switched to game phase {}", (int)this->phase);}


        // persistence
        json serialize() const;
        void deserialize(const json &j);
        void save(const std::filesystem::path &filepath) const;
        void load(const std::filesystem::path &filepath);

        // Tutorial
        void initTutorial();
        TutorialStep* getCurrentTutorialStep();
        void completeCurrentTutorialStep();
        bool isTutorialActive() const;
        bool isGameOver() const;

    private:
        // TODO: discuss ownership model for game...
        Graph map;

        std::vector<Player> players;

        // Smart pointer storage for safe ownership
        std::vector<std::shared_ptr<Settlement>> settlements;
        std::vector<std::shared_ptr<Road>> roads;

        // turns
        size_t currentPlayerId = 0;
        size_t turnCount = 0;
        size_t roundNumber = 0;
        types::GamePhase phase = types::GamePhase::START;
        Registry* registry = nullptr;
        // Tutorial
        std::vector<TutorialStep> tutorialSteps;
        size_t currentTutorialStep = 0;
    };

}
