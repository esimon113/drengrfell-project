#pragma once


#include <vector>
#include <map>

#include "tile.h"
#include "types.h"



namespace df{
    class Settlement;
    class ResourceType;
    class Road;
    class Tile;
    class Hero;

    class Player {
        private:
            size_t playerId;
            int heroPoints;
            int settlementCount;
            std::vector<Settlement*> settlements;
            std::map<types::TileType, int> resources;
            Hero* heroReference;
            std::vector<Road*> roads;
            std::vector<Tile*> exploredTiles;
        
        
        public:
            Player(size_t id);
            size_t getId() const;

            int getHeroPoints() const;
            void addHeroPoints(int points);
            void setHeroPoints(int points);

            int getSettlementCount() const;
            const std::vector<Settlement*>& getSettlement() const;
            void addSettlement(Settlement* settlement);
            void removeSettlement(Settlement* settlement);

            void addResources(types::TileType, int amount);
            void removeResources(types::TileType, int amount);
            int getResources(types::TileType, int amount) const;
            bool hasResources(const std::map<types::TileType, int>& amountRequired);
            const std::map<types::TileType, int>& getResources() const;

            void setHero(Hero* hero);
            Hero* getHero() const;

            void addRoad(Road* road);
            const std::vector<Road*>& getRoads() const;
            int getRoadCount() const;

            void exploreTile(Tile* tile);
            bool isTileExplored(const Tile* tile) const;
            bool isTileExplored(size_t tileId) const;
            const std::vector<Tile*>& getExploredTiles() const;


            void reset();

    };
}
