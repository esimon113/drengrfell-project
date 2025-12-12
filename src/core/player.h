#pragma once


#include <vector>
#include <map>
#include <memory>

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
            // We store IDs instead of references to respect ECS principles and serialize easily

            size_t playerId;
            int heroPoints;
            std::vector<size_t> settlementIds;
            std::map<types::TileType, int> resources;
            std::shared_ptr<Hero> heroReference;
            std::vector<size_t> roadIds;
            std::vector<size_t> exploredTileIds;
        
        
        public:
            Player();
            Player(size_t id);
            size_t getId() const;

            int getHeroPoints() const;
            void addHeroPoints(int );
            void setHeroPoints(int );

            // Changed to return IDs
            const std::vector<size_t>& getSettlementIds() const;
            void addSettlement(size_t settlementId);
            void removeSettlement(size_t settlementId);

            void addResources(types::TileType , int );
            void removeResources(types::TileType , int );
            int getResources(types::TileType type) const;            
            bool hasResources(const std::map<types::TileType, int>& );
            const std::map<types::TileType, int>& getResources() const;

            void setHero(std::shared_ptr<Hero> hero);
            std::shared_ptr<Hero> getHero() const;

            void addRoad(size_t roadId);
            const std::vector<size_t>& getRoadIds() const;
            int getRoadCount() const;

            void exploreTile(size_t tileId);
            bool isTileExplored(size_t tileId) const;
            const std::vector<size_t>& getExploredTileIds() const;

            size_t getPlayerId() const;
            void setPlayerId(size_t newPlayerId);

            const json serialize() const;

            void deserialize(const json& j);

            void reset();

    };
}