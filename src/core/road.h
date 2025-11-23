#pragma once

#include <vector>

namespace df {
    enum class RoadLevel : size_t {
        Path = 0,
        DirtRoad,
        StoneRoad,
        HighQualityRoad
    };

    class Road {
    public:
        Road();
        Road(size_t newId, size_t newPlayerId, size_t newEdgeId, RoadLevel newLevel, std::vector<int> newBuildingCost);

        ~Road();

        size_t getId() const;
        void setId(size_t newId);

        size_t getPlayerId() const;
        void setPlayerId(size_t newPlayerId);

        size_t getEdgeId() const;
        void setEdgeId(size_t newEdgeId);

        RoadLevel getRoadLevel() const;
        void setRoadLevel(RoadLevel newLevel);

        int getTradingBonus() const;
        void upgrade();

        const std::vector<int>& getBuildingCost() const;
        void setBuildingCost(std::vector<int> newBuildingCost);

    private:
        size_t id{ 0 };
        size_t playerId{ 0 };   // owned by player
        size_t edgeId{ 0 };     // placed on edge
        RoadLevel roadLevel{ RoadLevel::Path };
        std::vector<int> buildingCost{};
    };

}