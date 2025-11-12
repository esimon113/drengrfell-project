#include "edge.h"


namespace df {
	const json Edge::serialize() const {
		json j;

		// ...

		j["Test"] = 0;

		return j;
	}


	void Edge::deserialize(const json& j) {
		size_t newId = j["id"];
		this->setId(newId);

		// ...
	}
}
