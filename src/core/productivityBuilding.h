#pragma once

#include <cstddef>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace df {

	class ProductivityBuilding {
	  public:
		ProductivityBuilding();
		ProductivityBuilding(size_t newId, size_t newPlayerId, size_t newTileId);

		~ProductivityBuilding();

		size_t getId() const;
		void setId(size_t newId);

		size_t getPlayerId() const;
		void setPlayerId(size_t newPlayerId);

		size_t getTileId() const;
		void setTileId(size_t newTileId);

		const json serialize() const;
		void deserialize(const json& j);

	  private:
		size_t id{0};
		size_t playerId{0};
		size_t tileId{0};
	};

} // namespace df

