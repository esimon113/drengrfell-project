#pragma once

#include "types.h"

#include <vector>

namespace df {

	inline std::vector<int> makeResourceCost(int wood, int grass, int stone, int field, int clay) {
		std::vector<int> cost(static_cast<size_t>(types::TileType::COUNT), 0);
		cost[static_cast<size_t>(types::TileType::FOREST)] = wood;
		cost[static_cast<size_t>(types::TileType::GRASS)] = grass;
		cost[static_cast<size_t>(types::TileType::MOUNTAIN)] = stone;
		cost[static_cast<size_t>(types::TileType::FIELD)] = field;
		cost[static_cast<size_t>(types::TileType::CLAY)] = clay;
		return cost;
	}

	// Wood, wool, stone, grain, clay. These are the costs shown in the cost menu and the settlement menu.
	inline const std::vector<int>& settlementPlacementCost() {
		static const std::vector<int> cost = makeResourceCost(5, 3, 0, 3, 5);
		return cost;
	}

	inline const std::vector<int>& roadPlacementCost() {
		static const std::vector<int> cost = makeResourceCost(1, 0, 0, 0, 1);
		return cost;
	}

	inline const std::vector<int>& stoneSettlementUpgradeCost() {
		static const std::vector<int> cost = makeResourceCost(10, 10, 20, 10, 10);
		return cost;
	}

	inline const std::vector<int>& castleUpgradeCost() {
		static const std::vector<int> cost = makeResourceCost(30, 20, 50, 40, 30);
		return cost;
	}

	inline std::vector<int> productivityBuildingCost(types::TileType type) {
		switch (type) {
		case types::TileType::FOREST:
			return makeResourceCost(10, 0, 30, 0, 20);
		case types::TileType::MOUNTAIN:
			return makeResourceCost(30, 0, 10, 0, 20);
		case types::TileType::GRASS:
			return makeResourceCost(30, 0, 0, 10, 20);
		case types::TileType::FIELD:
			return makeResourceCost(20, 0, 20, 0, 20);
		case types::TileType::CLAY:
			return makeResourceCost(20, 10, 30, 0, 0);
		default:
			return makeResourceCost(0, 0, 0, 0, 0);
		}
	}

}
