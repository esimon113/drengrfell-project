#include "tile.h"


namespace df {
	const json Tile::serialize() const {
		json j;

		// ...

		j["Test"] = 0;

		return j;
	}


	void Tile::deserialize(const json& j) {
		size_t newId = j["id"];
		this->setId(newId);

		// ...
	}
}
