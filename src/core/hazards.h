#pragma once

#include "types.h"
#include <optional>
#include <string>
#include <unordered_map>

namespace df {

	// What hazard can occur on a tile, with which probability and if it is rendered on the map
	struct TileHazardProfile {
		types::HazardType hazardType;	// The type of the hazard
		float probability;				// The probability to encounter the hazard
		bool visibleBeforehand;			// If the hazard should be rendered on the map (currently unused)
	};

	// Properties of a hazard which are relevant for the game logic
	struct HazardDefinition {
		types::HazardType hazardType;	// The type of the hazard
		std::string name;				// The name of the hazard for displaying it in the game
		int defaultRoundDuration;		// How long the hazard holds the hero in place
		types::TileType skipRessource;	// Which ressource the player would have to pay to skip waiting
		std::string skipRessourceStr;	// A string which represents the ressource
		int skipCost;					// How many ressources it costs per round to skip the hazard
	};

	// Maps tileType to TileHazardProfile and hazardType to HazardDefinition
	class HazardDB {
	  public:
		static std::optional<TileHazardProfile> getTileHazardProfile(types::TileType tileType);

		static const HazardDefinition& getDefinition(types::HazardType type);

	  private:
		static std::unordered_map<types::HazardType, HazardDefinition> hazardDefinitions;
		static std::unordered_map<types::TileType, TileHazardProfile> tileHazardProfiles;
	};

} // namespace df
