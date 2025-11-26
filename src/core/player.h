#pragma once


#include <vector>
#include <map>

#include "tile.h"
#include "types.h"
#include "settlement.h"
#include <nlohmann/json.hpp>
#include "hero.h"
#include "road.h"

using json = nlohmann::json;


#include "hero.h"

namespace df{

    class Player {
        private:
            size_t playerId;
            int playerPoints;
            std::vector<size_t> settlements;
            std::map<types::TileType, int> resources;
            size_t heroReference;
            std::vector<size_t> roads;
            std::vector<size_t> exploredTiles;
        
        
        public:
            Player();
            Player(size_t id);
            size_t getId() const;

            int getPlayerPoints() const;
            void addPlayerPoints(int );
            void setPlayerPoints(int );

            void setHero(size_t);
            size_t getHero() const;

            const std::vector<size_t>& getSettlement() const;
            void addSettlement(size_t );

            void addResources(types::TileType , int );
            void removeResources(types::TileType , int );
            int getResources(types::TileType type) const;            
            bool hasResources(const std::map<types::TileType, int>& );
            const std::map<types::TileType, int>& getResources() const;

            void addRoad(size_t );
            const std::vector<size_t>& getRoads() const;

            void exploreTile(size_t );
            bool isTileExplored(size_t ) const;
            const std::vector<size_t>& getExploredTiles() const;

            const json serialize() const;

            void deserialize(const json& j);

            void reset();

    };
}
