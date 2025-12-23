#pragma once

#include <cstddef>
#include <optional>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "types.h"

namespace df {
	class Edge {
		public:
			Edge() = default;
			Edge(size_t id) : id(id) {};

			~Edge() = default;

			size_t getId() const { return this->id; }
			void setId(size_t newId) { this->id = newId; }

			bool hasRoad() const { return this->roadId.has_value(); }
			std::optional<size_t> getRoadId() const { return this->roadId; }
			void setRoadId(std::optional<size_t> newRoadId) { this->roadId = newRoadId; }

			types::EdgeDirection getDirection() const { return this->direction; }
			void setDirection(types::EdgeDirection newDirection) { this->direction = newDirection; }


			bool operator==(const Edge& other) const { return this->id == other.id; }


			// returns a json-representation of meta-info of an edge (id, ...).
			// This can then be used to store the complete topological state of the map.
			const json serialize() const;

			// Set meta-info by deserializing from provided json
			void deserialize(const json& j);


		private:
			size_t id;
			std::optional<size_t> roadId;
			types::EdgeDirection direction = types::EdgeDirection::VERTICAL;	// Standard direction, needs to be set inside the construction once the map layout is decided in the team
	};


	using EdgeHandle = Edge*;
}
