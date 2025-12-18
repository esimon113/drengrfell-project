#include <map>
#include <optional>

#include "hero.h"
#include "gamecontroller.h"





namespace df {

    Player* GameController::getCurrentPlayer() { return this->getPlayerbyId(this->gameState.getCurrentPlayerId()); }
    const Player* GameController::getCurrentPlayer() const { return this->getPlayerById(this->gameState.getCurrentPlayerId()); }


    Player* GameController::getPlayerbyId(size_t playerId) { return this->gameState.getPlayer(playerId); }
    const Player* GameController::getPlayerById(size_t playerId) const { return this->gameState.getPlayer(playerId); }


    void GameController::startTurn() {
        Player* player = this->getCurrentPlayer();
        if (!player) { return; }

        this->giveResourcesTo(*player);
        this->resetHeroMovement(*player);
    }


    void GameController::endTurn() {
        const size_t playerCount = this->gameState.getPlayerCount();
        if (playerCount == 0) { return; } // should not happen

        // TODO: maybe add some "setNextTurn()" etc. functions
        size_t nextPlayerId = (this->gameState.getCurrentPlayerId() + 1) % playerCount;
        this->gameState.setCurrentPlayerId(nextPlayerId);
        this->gameState.setTurnCount(this->gameState.getTurnCount() + 1);

        if (nextPlayerId == 0) {
            this->gameState.setRoundNumber(this->gameState.getRoundNumber() + 1);
        }
    }


    void GameController::giveResourcesTo(Player& player) {
        // resources are given to the player based on the settlements they have
        // for testing purposes, players receive some resources every turn
        bool test = true;
        if (test) {
            player.addResources(types::TileType::FOREST, 1);
            player.addResources(types::TileType::CLAY, 1);
            player.addResources(types::TileType::GRASS, 1);
            player.addResources(types::TileType::FIELD, 1);
            player.addResources(types::TileType::MOUNTAIN, 1);

            return;
        }

        for (size_t settlementId : player.getSettlementIds()) {
            const Settlement* settlement = this->findSettlementById(settlementId);
            if (!settlement) { continue; }

            const auto tileIds = this->getSettlementTiles(*settlement);
            for (size_t tileId : tileIds) {
                const Tile& tile = this->gameState.getMap().getTile(tileId);
                if (tile.givesResourceThisTurn(this->rng)) {
                    player.addResources(tile.getType(), 1); // TODO: make amount configurable -> i.e. in settlers of catan a town gives 2 resources
                }
            }
        }
    }


    void GameController::resetHeroMovement(Player& player) {
        std::shared_ptr<Hero> hero = player.getHero();
        if (hero) {
            // TODO: something like this needs to be implemented in hero class:
            // reset the available movement points, hero also needs to keep track of used range per turn
            // hero->resetMovementPoints();
            //hero->startIdleAnimation();   
        }
    }


    void GameController::exploreTile(Player& player, size_t tileId) {
        Graph& map = this->gameState.getMap();

        try {
            Tile& tile = map.getTile(tileId);

            if (!player.isTileExplored(tileId)) {
                tile.addVisibleForPlayers(player.getId());
                player.exploreTile(tileId);
            }
        } catch (const std::exception&) {} // invalid tile -> ignore
    }


    bool GameController::moveHeroToTile(size_t playerId, size_t targetTileId) {
        Player* player = this->getPlayerbyId(playerId);
        if (!player) { return false; }

        std::shared_ptr<Hero> hero = player->getHero();
        if (!hero) { return false; }

        const int currentTileId = hero->getTileID(); // TODO: use size_t in hero
        size_t distance = 0;
        if (currentTileId >= 0) {
            const auto& map = this->gameState.getMap();
            auto currentTile = map.getTile(currentTileId);
            auto targetTile = map.getTile(targetTileId);
            distance = map.getDistanceBetween(currentTile, targetTile);

            if (distance == SIZE_MAX) { return false; }
        }

        // TODO: hero class should implement moving the hero to a specified tile.
        // TODO: use size_t in hero -> id and distance cannot be negative -> make this information explicit by used datatype
        // if (!hero->moveToTile(targetTileId, distance)) { return false; }

        auto range = hero->getBaseRange();
        auto remainingRange = range - static_cast<int>(distance);
        // TODO: set remaining range for the hero for the current turn
        // otherwise we need to specify that the hero can be moved only once per turn
        // something like this:
        // hero->setRemainingRange(remainingRange);

        // TODO: this is only a temporary workaround:
        if (remainingRange < 0) { return false; }

        this->exploreTile(*player, targetTileId);

        return true; // success
    }


    bool GameController::canBuildSettlement(size_t playerId, size_t vertexId) const {
        (void)playerId; // unused for now - simplified building rules
        const Graph& map = this->gameState.getMap();
        try {
            // Find vertex by ID (not index)
            const Vertex* vertex = nullptr;
            for (size_t i = 0; i < map.getVertexCount(); ++i) {
                if (map.getVertex(i).getId() == vertexId) {
                    vertex = &map.getVertex(i);
                    break;
                }
            }
            if (!vertex) { return false; }
            
            // Only check if vertex already has a settlement
            if (vertex->hasSettlement()) { return false; }
            // Also check that no adjacent vertices have settlements (basic rule)
            if (this->doesVertexHaveNeighborSettlements(vertexId)) { return false; }
            return true;
        } catch (const std::exception&) { return false; }
    }


    bool GameController::buildSettlement(size_t playerId, size_t vertexId, const std::vector<int>& buildingCost) {
        if (!this->canBuildSettlement(playerId, vertexId)) { 
            return false; 
        }

        Player* player = this->getPlayerbyId(playerId);
        if (!player) { 
            return false; 
        }
        if (!this->hasEnoughResources(*player, buildingCost)) { 
            return false; 
        }

        Graph& map = this->gameState.getMap();

        try {
            // Find vertex by ID (not index) - vertexId is the ID stored in the Vertex object
            Vertex* vertex = nullptr;
            for (size_t i = 0; i < map.getVertexCount(); ++i) {
                if (map.getVertex(i).getId() == vertexId) {
                    vertex = &map.getVertex(i);
                    break;
                }
            }
            
            if (!vertex) {
                return false; // Vertex with this ID not found
            }
            
            // Double-check vertex doesn't already have a settlement (race condition protection)
            if (vertex->hasSettlement()) {
                return false;
            }
            
            size_t newSettlementId = this->gameState.getSettlements().size();
            auto newSettlement = std::make_shared<Settlement>(newSettlementId, playerId, vertexId, buildingCost);

            vertex->setSettlementId(newSettlementId);
            this->gameState.addSettlement(newSettlement);
            player->addSettlement(newSettlement->getId());

            this->chargeResourceCost(*player, buildingCost);

            return true;

        } catch (const std::exception& e) { 
            return false; 
        }
    }


    // TODO: validate this in edge class
    bool GameController::canBuildRoad(size_t playerId, size_t edgeId) const {
        (void)playerId; // unused for now - simplified building rules
        const Graph& map = this->gameState.getMap();

        try {
            // Find edge by ID (not index)
            const Edge* edge = nullptr;
            for (size_t i = 0; i < map.getEdgeCount(); ++i) {
                if (map.getEdge(i).getId() == edgeId) {
                    edge = &map.getEdge(i);
                    break;
                }
            }
            if (!edge) { return false; }
            
            // Only check if edge already has a road
            if (edge->hasRoad()) { return false; }
            return true;
        } catch (const std::exception&) { return false; }
    }


    bool GameController::buildRoad(size_t playerId, size_t edgeId, RoadLevel level, const std::vector<int>& buildingCost) {
        if (!this->canBuildRoad(playerId, edgeId)) { return false; }

        Player* player = this->getPlayerbyId(playerId);
        if (!player) { return false; }

        if (!this->hasEnoughResources(*player, buildingCost)) { return false; }

        Graph& map = this->gameState.getMap();
        try {
            // Find edge by ID (not index)
            Edge* edge = nullptr;
            for (size_t i = 0; i < map.getEdgeCount(); ++i) {
                if (map.getEdge(i).getId() == edgeId) {
                    edge = &map.getEdge(i);
                    break;
                }
            }
            if (!edge) { return false; }
            
            // Double-check edge doesn't already have a road
            if (edge->hasRoad()) { return false; }
            
            size_t roadId = this->gameState.getRoads().size();
            auto road = std::make_shared<Road>(roadId, playerId, edgeId, level, buildingCost);

            edge->setRoadId(roadId);
            this->gameState.addRoad(road);
            player->addRoad(road->getId());

            this->chargeResourceCost(*player, buildingCost);

            return true;

        } catch (const std::exception&) {
            return false;
        }
    }


    // TODO: move this functionality to settlement class
    std::vector<size_t> GameController::getSettlementTiles(const Settlement& settlement) const {
        std::vector<size_t> tileIds;
        const Graph& map = this->gameState.getMap();
        try {
            // Find vertex by ID (not index)
            const Vertex* vertex = nullptr;
            for (size_t i = 0; i < map.getVertexCount(); ++i) {
                if (map.getVertex(i).getId() == settlement.getVertexId()) {
                    vertex = &map.getVertex(i);
                    break;
                }
            }
            if (!vertex) { return tileIds; }
            
            const auto tiles = map.getVertexTiles(*vertex);
            for (const auto& tile : tiles) {
                if (tile.getId() != SIZE_MAX) {
                    tileIds.push_back(tile.getId());
                }
            }
        } catch (const std::exception&) {} // ignore invalid vertex

        return tileIds;
    }


    // TODO: discuss where to put this...
    // Put this into vertex class? -> or better in settlement class as "hasNeighbourSettlement()"?!
    bool GameController::doesVertexHaveNeighborSettlements(size_t vertexId) const {
        const Graph& map = this->gameState.getMap();

        try {
            // Find vertex by ID (not index)
            const Vertex* vertex = nullptr;
            for (size_t i = 0; i < map.getVertexCount(); ++i) {
                if (map.getVertex(i).getId() == vertexId) {
                    vertex = &map.getVertex(i);
                    break;
                }
            }
            if (!vertex) { return true; }
            
            const auto edges = map.getVertexEdges(*vertex);
            for (const auto& edge : edges) {
                if (edge.getId() == SIZE_MAX) { continue; }

                const auto vertices = map.getEdgeVertices(edge);
                for (const auto& neighbour : vertices) {
                    if (neighbour.getId() == SIZE_MAX || neighbour.getId() == vertexId) { continue; }
                    if (neighbour.hasSettlement()) { return true; }
                }
            }
        } catch (const std::exception&) { return true; }

        return false;
    }


    // check if edge is connected with roads to a settlement from the player:
    // either the edge is directly connected to a settlement form the player
    // or the edge is connected to a road -> a road is always connected to a settlement
    bool GameController::doesEdgeConnectToPlayer(size_t playerId, size_t edgeId) const {
        const Graph& map = this->gameState.getMap();

        try {
            // Find edge by ID (not index)
            const Edge* edge = nullptr;
            for (size_t i = 0; i < map.getEdgeCount(); ++i) {
                if (map.getEdge(i).getId() == edgeId) {
                    edge = &map.getEdge(i);
                    break;
                }
            }
            if (!edge) { return false; }
            
            const auto vertices = map.getEdgeVertices(*edge);

            for (const auto& vertex : vertices) {
                if (vertex.hasSettlement()) {

                    const auto settlementId = vertex.getSettlementId();
                    if (settlementId.has_value()) {

                        const Settlement* settlement = this->findSettlementById(settlementId.value());
                        if (settlement && settlement->getPlayerId() == playerId) { return true; }
                    }
                }

                const auto edges = map.getVertexEdges(vertex);
                for (const auto& neighbourEdge : edges) {
                    if (neighbourEdge.getId() == SIZE_MAX || neighbourEdge.getId() == edgeId || !neighbourEdge.hasRoad()) {
                        continue;
                    }
                    const auto roadId = neighbourEdge.getRoadId();
                    if (!roadId.has_value()) { continue; }

                    const Road* road = this->findRoadById(roadId.value());
                    if (road && road->getPlayerId() == playerId) { return true; }
                }
            }
        } catch (const std::exception&) { return false; }

        return false;
    }


    // TODO: changing buildingCost to a map (as is planned), would make this function rather obsolete
    bool GameController::hasEnoughResources(Player& player, const std::vector<int>& buildingCost) {
        if (buildingCost.empty()) { return true; } // building is free

        std::map<types::TileType, int> requirements;
        for (size_t i = 0; i < buildingCost.size() && i < static_cast<size_t>(types::TileType::COUNT); ++i) {
            if (buildingCost[i] > 0) {
                requirements[static_cast<types::TileType>(i)] = buildingCost[i];
            }
        }

        return (requirements.empty() || player.hasResources(requirements));
    }


    // TODO: when buildingCost is a map, this function would not be necessary anymore
    void GameController::chargeResourceCost(Player& player, const std::vector<int>& buildingCost) {
        if (buildingCost.empty()) { return; } // building is free -> nothing charged

        for (size_t i = 0; i < buildingCost.size() && i < static_cast<size_t>(types::TileType::COUNT); ++i) {
            if (buildingCost[i] > 0) {
                player.removeResources(static_cast<types::TileType>(i), buildingCost[i]);
            }
        }
    }


    // util functions
    const Road* GameController::findRoadById(size_t roadId) const {
        const auto& roads = this->gameState.getRoads();

        for (const auto& road : roads) {
            if (road && road->getId() == roadId) {
                return road.get();
            }
        }

        return nullptr;
    }


    // util functions
    const Settlement* GameController::findSettlementById(size_t settlementId) const {
        const auto& settlements = this->gameState.getSettlements();

        for (const auto& settlement : settlements) {
            if (settlement && settlement->getId() == settlementId) {
                return settlement.get();
            }
        }

        return nullptr;
    }

}