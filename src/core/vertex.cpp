#include "vertex.h"


namespace df {
	const json Vertex::serialize() const {
		json j;

		j["id"] = id;
		if (settlementId.has_value())
			j["settlementId"] = settlementId.value();
		else
			j["settlementId"] = nullptr;

		return j;
	}


	void Vertex::deserialize(const json& j) {
		size_t newId = j["id"];
		this->setId(newId);

		if (!j["settlementId"].is_null()) {
			size_t newSettlementId = j["settlementId"].get<size_t>();
			this->setSettlementId(newSettlementId);
		}
		else {
			this->setSettlementId(std::nullopt);
		}
	}
}
