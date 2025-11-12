#pragma once

#include <cstddef>


#include <nlohmann/json.hpp>
using json = nlohmann::json;



namespace df {
	class Edge {
		public:
			Edge() = default;
			Edge(size_t id) : id(id) {};

			~Edge() = default;

			size_t getId() const { return this->id; }
			void setId(size_t newId) { this->id = newId; }


			bool operator==(const Edge& other) const { return this->id == other.id; }


			// returns a json-representation of meta-info of an edge (id, ...).
			// This can then be used to store the complete topological state of the map.
			const json serialize() const;

			// Set meta-info by deserializing from provided json
			void deserialize(const json& j);


		private:
			size_t id;
	};
}
