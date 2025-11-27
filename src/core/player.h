#pragma once


#include <vector>
#include <map>

#include "tile.h"
#include "types.h"
#include "settlement.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include "hero.h"
#include "road.h"



namespace df{

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
            Player();
            Player(size_t id);
            size_t getId() const;

            int getHeroPoints() const;
            void addHeroPoints(int );
            void setHeroPoints(int );

            const std::vector<Settlement*>& getSettlement() const;
            void addSettlement(Settlement* );
            void removeSettlement(Settlement* );

            void addResources(types::TileType , int );
            void removeResources(types::TileType , int );
            int getResources(types::TileType type) const;            
            bool hasResources(const std::map<types::TileType, int>& );
            const std::map<types::TileType, int>& getResources() const;

            void setHero(Hero* );
            Hero* getHero() const;

            void addRoad(Road* );
            const std::vector<Road*>& getRoads() const;
            int getRoadCount() const;

            void exploreTile(Tile* );
            bool isTileExplored(const Tile* ) const;
            bool isTileExplored(size_t ) const;
            const std::vector<Tile*>& getExploredTiles() const;

            size_t getPlayerId() const;
            void setPlayerId(size_t newPlayerId);

            const json serialize() const;

            void deserialize(const json& j);

            void reset();

    };
}
