#include "vertex.h"


namespace df {
	const json Vertex::serialize() const {
		json j;

		// ...

		j["Test"] = 0;

		return j;
	}


	void Vertex::deserialize(const json& j) {
		size_t newId = j["id"];
		this->setId(newId);

		// ...
	}
}
