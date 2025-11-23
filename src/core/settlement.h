#pragma once

#include <vector>

namespace df {

    class Settlement {
    public:
        Settlement();
        Settlement(size_t newId, size_t newPlayerId, size_t vertexId, std::vector<int> newBuildingCost);

        ~Settlement();

        size_t getId() const;
        void setId(size_t newId);

        size_t getPlayerId() const;
        void setPlayerId(size_t newPlayerId);

        size_t getVertexId() const;
        void setVertexId(size_t newVertexId);

        const std::vector<int>& getBuildingCost() const;
        void setBuildingCost(std::vector<int> newBuildingCost);

    private:
        size_t id{0};
        size_t playerId{0};
        size_t vertexId{ 0 };
        std::vector<int> buildingCost{};
    };

}