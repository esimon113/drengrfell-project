#pragma once

#include <cstddef>
#include <optional>

#include <nlohmann/json.hpp>
using json = nlohmann::json;



namespace df {
	class Vertex {
		public:
			Vertex() = default;
			Vertex(size_t id) : id(id) {};

			~Vertex() = default;

			size_t getId() const { return this->id; }
			void setId(size_t newId) { this->id = newId; }

			bool hasSettlement() const { return this->settlementId.has_value(); }
			std::optional<size_t> getSettlementId() const { return this->settlementId; }
			void setSettlementId(std::optional<size_t> newSettlementId) { this->settlementId = newSettlementId; }


			bool operator==(const Vertex& other) const { return this->id == other.id; }


			// returns a json-representation of meta-info of a vertex (id, ...).
			// This can then be used to store the complete topological state of the map.
			const json serialize() const;

			// Set meta-info by deserializing from provided json
			void deserialize(const json& j);


		private:
			size_t id;
			std::optional<size_t> settlementId;
	};


	using VertexHandle = Vertex*;
}
