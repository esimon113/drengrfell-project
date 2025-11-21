#pragma once

#include <cstddef>
#include <tuple>


namespace df::types {

	// maybe add more custom tile types in the future?!
	enum class TileType {
		EMPTY = 0,
		WATER,
		FOREST,
		GRASS,
		MOUNTAIN,
		FIELD,
		CLAY,
		ICE
	};


	// maybe like 10/25/50% chance to get resource per round?!
	enum class TilePotency {
		LOW,
		MEDIUM,
		HIGH
	};


	enum class TileDirection {
		NORTH = 0,
        NORTH_WEST,
        SOUTH_WEST,
        SOUTH,
        SOUTH_EAST,
        NORTH_EAST
	};


    inline std::tuple<size_t, size_t> getTileDirectionCoordinates(TileDirection direction) {
        switch (direction) {
            case TileDirection::NORTH:       return { 0, 1 };
            case TileDirection::SOUTH:       return { 0, -1 };
            case TileDirection::NORTH_EAST:  return { 1, 1 };
            case TileDirection::NORTH_WEST:  return { -1, 1 };
            case TileDirection::SOUTH_EAST:  return { 1, -1 };
            case TileDirection::SOUTH_WEST:  return { -1, -1 };
            default: return { 0, 0 };
        }
    }
}
