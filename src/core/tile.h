#pragma once

#include <cstddef>
#include <vector>
#include <optional>
#include <random>

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

			bool hasBuilding() const { return this->buildingId.has_value(); }
			std::optional<size_t> getBuildingId() const { return this->buildingId; }
			void setBuildingId(std::optional<size_t> newBuildingId) { this->buildingId = newBuildingId; }

			const std::vector<size_t>& getVisibleForPlayers() const { return this->visibleForPlayers; }
			void setVisibleForPlayers(const std::vector<size_t>& playerIds) { this->visibleForPlayers = playerIds; }
			void addVisibleForPlayers(size_t playerId) { this->visibleForPlayers.push_back(playerId); }

			float getRangeFactor() const { return this->rangeFactor; }
			void setRangeFactor(float range) { this->rangeFactor = range; }

			// Determines if this tile gives a resource this turn, based on the tile's type and potency.
			bool givesResourceThisTurn(std::mt19937& rng) const;

			bool operator==(const Tile& other) const { return this->id == other.id; }


			// returns a json-representation of meta-info of a tile (id, type, ...).
			// This can then be used to store the complete topological state of the map.
			const json serialize() const;

			// Set meta-info by deserializing from provided json
			void deserialize(const json& j);


		private:
			size_t id;
			types::TileType type;	// Also acts as resource type (?)
			types::TilePotency potency;
			std::optional<size_t> buildingId;
			std::vector<size_t> visibleForPlayers;
			float rangeFactor = 1.0f;

			bool isResourceTile() const;
			float getPotencyProbability(types::TilePotency potency) const;
	};


	using TileHandle = Tile*;
}
