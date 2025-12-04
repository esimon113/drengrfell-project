#pragma once

#include <cstddef>
#include <tuple>


namespace df::types {

	// maybe add more custom tile types in the future?!
	// Attention, dear traveller:
	//     render.cpp, tile.frag.glsl and the tileAtlas.png have to be updated when adding tiles
	enum class TileType: int {
		EMPTY = 0,
		WATER,
		FOREST,
		GRASS,
		MOUNTAIN,
		FIELD,
		CLAY,
		ICE,
		COUNT
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

	enum class EdgeDirection {
		VERTICAL = 0,
		DIAGONAL_DOWN,	// NORTH - NORTH_EAST or SOUTH_WEST - SOUTH
		DIAGONAL_UP		// NORTH_WEST - NORTH or SOUTH - SOUTH_EAST
	};


	/*
	 * `START`: Show start screen -> Shows nice background + buttons to start game / exit program / etc.
	 * `CONFIG`: Configure game settings -> entered if "start a game" is selected in Start Screen (players are configured here)
	 * `PLAY`: Actual gameplay phase with turn-based game loop -> started if "start" is selected in config phase
	 * `END`: End screen -> Shows some game stats (points, ...) -> entered if game is won/lost or ended by user
	 */
    enum class GamePhase {
        START,
        CONFIG,
        PLAY,
        END
    };
}
