#include "tile.h"


namespace df {
	const json Tile::serialize() const {
		json j;

		j["id"] = id;
		// static_cast turns the enums into ints so they are json-compatible
		j["type"] = static_cast<int>(type);
		j["potency"] = static_cast<int>(potency);
		j["rangeFactor"] = rangeFactor;

		if (buildingId.has_value())
			j["buildingId"] = buildingId.value();
		else
			j["buildingId"] = nullptr;

		j["visibleForPlayers"] = visibleForPlayers;

		return j;
	}


	void Tile::deserialize(const json& j) {
		size_t newId = j["id"];
		this->setId(newId);

		int typeInt = j["type"].get<int>();
		types::TileType newType = static_cast<types::TileType>(typeInt);
		this->setType(newType);

		int potencyInt = j["potency"].get<int>();
		types::TilePotency newPotency = static_cast<types::TilePotency>(potencyInt);
		this->setPotency(newPotency);

		float newRange = j["rangeFactor"];
		this->setRangeFactor(newRange);

		if (!j["id"].is_null()) {
			std::optional<size_t> newBuildingId = j["buildingId"];
			this->setBuildingId(newBuildingId);
		}
		else {
			buildingId.reset();
		}

		std::vector<size_t> newVisibleForPlayers = j["visibleForPlayers"];
		this->setVisibleForPlayers(newVisibleForPlayers);

	}
}
