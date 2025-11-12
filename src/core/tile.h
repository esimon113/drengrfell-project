#pragma once

#include <cstddef>

#include <nlohmann/json.hpp>
using json = nlohmann::json;


#include "types.h"



namespace df {

	class Tile {
		public:
			Tile() = default;
			Tile(size_t id, types::TileType type, types::TilePotency potency)
				: id(id), type(type), potency(potency) {};

			~Tile() = default;

			size_t getId() const { return this->id; }
			void setId(size_t newId) { this->id = newId; }

			types::TileType getType() const { return this->type; }
			void setType(types::TileType newType) { this->type = newType; }

			types::TilePotency getPotency() const { return this->potency; }
			void setPotency(types::TilePotency newPotency) { this->potency = newPotency; }


			bool operator==(const Tile& other) const { return this->id == other.id; }


			// returns a json-representation of meta-info of a tile (id, type, ...).
			// This can then be used to store the complete topological state of the map.
			const json serialize() const;

			// Set meta-info by deserializing from provided json
			void deserialize(const json& j);


		private:
			size_t id;
			types::TileType type;
			types::TilePotency potency;
			// ...
	};
}
