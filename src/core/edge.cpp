#include "edge.h"


namespace df {
	const json Edge::serialize() const {
		json j;

		j["id"] = id;
		if (roadId.has_value())
			j["roadId"] = roadId.value();
		else
			j["roadId"] = nullptr;

		j["direction"] = static_cast<int>(direction);

		return j;
	}


	void Edge::deserialize(const json& j) {
		size_t newId = j["id"];
		this->setId(newId);

		if (!j["roadId"].is_null()) {
			size_t newRoadId = j["roadId"].get<size_t>();
			this->setRoadId(newRoadId);
		}
		else {
			this->setRoadId(std::nullopt);
		}

		int directionInt = j["direction"].get<int>();
		types::EdgeDirection newDirection = static_cast<types::EdgeDirection>(directionInt);
		this->setDirection(newDirection);
	}
}
