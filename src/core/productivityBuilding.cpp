#include "productivityBuilding.h"

namespace df {

	ProductivityBuilding::ProductivityBuilding() = default;

	ProductivityBuilding::ProductivityBuilding(size_t id, size_t playerId, size_t tileId)
		: id(id), playerId(playerId), tileId(tileId) {
	}

	ProductivityBuilding::~ProductivityBuilding() = default;

	size_t ProductivityBuilding::getId() const { return id; }
	void ProductivityBuilding::setId(size_t newId) { id = newId; }

	size_t ProductivityBuilding::getPlayerId() const { return playerId; }
	void ProductivityBuilding::setPlayerId(size_t newPlayerId) { playerId = newPlayerId; }

	size_t ProductivityBuilding::getTileId() const { return tileId; }
	void ProductivityBuilding::setTileId(size_t newTileId) { tileId = newTileId; }

	const json ProductivityBuilding::serialize() const {
		json j;
		j["id"] = id;
		j["playerId"] = playerId;
		j["tileId"] = tileId;
		return j;
	}

	void ProductivityBuilding::deserialize(const json& j) {
		this->setId(j.at("id").get<size_t>());
		this->setPlayerId(j.at("playerId").get<size_t>());
		this->setTileId(j.at("tileId").get<size_t>());
	}

} // namespace df

