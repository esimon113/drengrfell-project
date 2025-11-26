#include "settlement.h"

namespace df {

    Settlement::Settlement() = default;

    Settlement::Settlement(size_t id, size_t playerId, size_t vertexId, std::vector<int> buildingCost)
        : id(id), playerId(playerId), vertexId(vertexId), buildingCost(buildingCost) {
    }

    Settlement::~Settlement() = default;

    size_t Settlement::getId() const { return id; }
    void Settlement::setId(size_t newId) { id = newId; }

    size_t Settlement::getPlayerId() const { return playerId; }
    void Settlement::setPlayerId(size_t newPlayerId) { playerId = newPlayerId; }

    size_t Settlement::getVertexId() const { return vertexId; }
    void Settlement::setVertexId(size_t newVertexId) { vertexId = newVertexId; }

    const std::vector<int>& Settlement::getBuildingCost() const { return buildingCost; }
    void Settlement::setBuildingCost(std::vector<int> newBuildingCost) { buildingCost = newBuildingCost; }


	const json Settlement::serialize() const {
		json j;

		j["id"] = id;
		j["playerId"] = playerId;
		j["vertexId"] = vertexId;
		j["buildingCost"] = buildingCost;

		return j;
	}

    void Settlement::deserialize(const json& j) {
        this->setId(j.at("id").get<size_t>());
        this->setPlayerId(j.at("playerId").get<size_t>());
        this->setVertexId(j.at("vertexId").get<size_t>());
        this->setBuildingCost(j.at("buildingCost").get<std::vector<int>>());
    }

}
