#pragma once

#include <cstddef>
#include <string>
#include <tuple>


namespace df::types {

	// maybe add more custom tile types in the future?!
	// Attention, dear traveller:
	//     render.cpp, tile.frag.glsl and the tileAtlas.png have to be updated when adding tiles
	enum class TileType : int {
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

	inline std::string tileTypeToString(TileType t) {
		switch (t) {
		case TileType::WATER:
			return "WATER";
		case TileType::FOREST:
			return "FOREST";
		case TileType::GRASS:
			return "GRASS";
		case TileType::MOUNTAIN:
			return "MOUNTAIN";
		case TileType::FIELD:
			return "FIELD";
		case TileType::CLAY:
			return "CLAY";
		case TileType::ICE:
			return "ICE";
		default:
			return "EMPTY";
		};
	}


	// maybe like 10/25/50% chance to get resource per round?!
	enum class TilePotency {
		LOW,
		MEDIUM,
		HIGH
	};


	inline std::string potencyToString(TilePotency p) {
		switch (p) {
		case TilePotency::LOW:
			return "LOW";
		case TilePotency::MEDIUM:
			return "MEDIUM";
		case TilePotency::HIGH:
			return "HIGH";
		default:
			return "";
		};
	}


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
		case TileDirection::NORTH:
			return {0, 1};
		case TileDirection::SOUTH:
			return {0, -1};
		case TileDirection::NORTH_EAST:
			return {1, 1};
		case TileDirection::NORTH_WEST:
			return {-1, 1};
		case TileDirection::SOUTH_EAST:
			return {1, -1};
		case TileDirection::SOUTH_WEST:
			return {-1, -1};
		default:
			return {0, 0};
		}
	}

	enum class QuestGoalType {
		TUTORIAL,
		SETTLEMENT,
		ROAD,
		WATER,
		FOREST,
		GRASS,
		MOUNTAIN,
		FIELD,
		CLAY,
		ICE,
		ROUNDS,
		NONE
	};

	inline QuestGoalType tileToQuestGoal(TileType tileType) {
        switch (tileType) {
            case TileType::FOREST:   return QuestGoalType::FOREST;
            case TileType::CLAY:     return QuestGoalType::CLAY;
            case TileType::MOUNTAIN: return QuestGoalType::MOUNTAIN;
            case TileType::FIELD:    return QuestGoalType::FIELD;
            case TileType::GRASS:    return QuestGoalType::GRASS;
            case TileType::WATER:    return QuestGoalType::WATER;
            case TileType::ICE:      return QuestGoalType::ICE;
            default:                 return QuestGoalType::NONE;
        }
    }

	enum class EdgeDirection {
		VERTICAL = 0,
		DIAGONAL_DOWN, // NORTH - NORTH_EAST or SOUTH_WEST - SOUTH
		DIAGONAL_UP	   // NORTH_WEST - NORTH or SOUTH - SOUTH_EAST
	};


	enum class GamePhase {
		START,
		CONFIG,
		PLAY,
		END
	};

	enum class HazardType {
		NONE,
		MUD,
		ROCKSLIDE,
		BEAR,
		BLIZZARD
	};

	
} // namespace df::types
