#include "road.h"

namespace df {

    Road::Road() = default;
    Road::Road(size_t id, size_t playerId, size_t edgeId, RoadLevel roadLevel, std::vector<int> buildingCost)
        : id(id), playerId(playerId), edgeId(edgeId), roadLevel(roadLevel), buildingCost(buildingCost) {
    }

    Road::~Road() = default;

    size_t Road::getId() const { return id; }
    void Road::setId(size_t newId) { id = newId; }

    size_t Road::getPlayerId() const { return playerId; }
    void Road::setPlayerId(size_t newPlayerId) { playerId = newPlayerId; }

    size_t Road::getEdgeId() const { return edgeId; }
    void Road::setEdgeId(size_t newEdgeId) { edgeId = newEdgeId; }

    RoadLevel Road::getRoadLevel() const { return roadLevel; }
    void Road::setRoadLevel(RoadLevel newLevel) { roadLevel = newLevel; }

    const std::vector<int>& Road::getBuildingCost() const { return buildingCost; }
    void Road::setBuildingCost(std::vector<int> newBuildingCost) { buildingCost = newBuildingCost; }

    // Upgrade roads to next higher tier
    void Road::upgrade() {
        switch (roadLevel) {
        case RoadLevel::Path:          roadLevel = RoadLevel::DirtRoad; break;
        case RoadLevel::DirtRoad:      roadLevel = RoadLevel::StoneRoad; break;
        case RoadLevel::StoneRoad:     roadLevel = RoadLevel::HighQualityRoad; break;
        // alredy maxed out
        case RoadLevel::HighQualityRoad: return; 
        }
    }

    // Trade bonus acording to roadlevel
    int Road::getTradingBonus() const {
        switch (roadLevel) {
        case RoadLevel::Path:           return 1;
        case RoadLevel::DirtRoad:       return 2;
        case RoadLevel::StoneRoad:      return 3;
        case RoadLevel::HighQualityRoad:return 4;
        }
        // fallback
        return 0;
    }

    const json Road::serialize() const {
        json j;

        j["id"] = id;
        j["playerId"] = playerId;
        j["edgeId"] = edgeId;
        j["roadLevel"] = static_cast<size_t>(roadLevel);
        j["buildingCost"] = buildingCost;

        return j;
    }

    void Road::deserialize(const json& j) {
        this->setId(j.at("id").get<size_t>());
        this->setPlayerId(j.at("playerId").get<size_t>());
        this->setEdgeId(j.at("edgeId").get<size_t>());
        this->setRoadLevel(static_cast<RoadLevel>(j.at("roadLevel").get<size_t>()));
        this->setBuildingCost(j.at("buildingCost").get<std::vector<int>>());
    }
}
