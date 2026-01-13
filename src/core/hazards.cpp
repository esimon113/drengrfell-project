#include "hazards.h"

namespace df {

	// mapping between HazardType and HazardDefinition
	/*
	A HazardDefinition includes the following fields:
	types::HazardType hazardType;	// The type of the hazard
	std::string name;				// The name of the hazard for displaying it in the game
	int defaultRoundDuration;		// How long the hazard holds the hero in place
	types::TileType skipRessource;	// Which ressource the player would have to pay to skip waiting
	int skipCost;					// How many ressources it costs per round to skip the hazard
	*/
	std::unordered_map<types::HazardType, HazardDefinition> HazardDB::hazardDefinitions = {
		{types::HazardType::NONE, {types::HazardType::NONE, "None", 0, types::TileType::EMPTY, 0}},
		{types::HazardType::MUD, {types::HazardType::MUD, "Sticky mud pit", 2, types::TileType::FOREST, 1}},
		{types::HazardType::ROCKSLIDE, {types::HazardType::ROCKSLIDE, "Rockslide", 3, types::TileType::FOREST, 2}},
		{types::HazardType::BLIZZARD, {types::HazardType::BLIZZARD, "Blizzard", 2, types::TileType::FIELD, 3}},
		{types::HazardType::BEAR, {types::HazardType::BEAR, "Bear", 1, types::TileType::FIELD, 1}},
	};

	const HazardDefinition& HazardDB::getDefinition(types::HazardType type) {
		return hazardDefinitions.at(type);
	}

	// mapping between TileType and TileHazardProfile
	/*
	A TileHazardProfile includes the following fields:
	types::HazardType hazardType;	// The type of the hazard
	float probability;				// The probability to encounter the hazard
	bool visibleBeforehand;			// If the hazard should be rendered on the map (currently unused)
	*/
	std::unordered_map<types::TileType, TileHazardProfile> HazardDB::tileHazardProfiles = {
		{types::TileType::WATER, {types::HazardType::NONE, 0.3f, false}},
		{types::TileType::FOREST, {types::HazardType::BEAR, 0.25f, false}},
		{types::TileType::GRASS, {types::HazardType::NONE, 0.3f, false}},
		{types::TileType::MOUNTAIN, {types::HazardType::ROCKSLIDE, 0.4f, false}},
		{types::TileType::FIELD, {types::HazardType::NONE, 0.3f, false}},
		{types::TileType::CLAY, {types::HazardType::MUD, 0.3f, false}},
		{types::TileType::ICE, {types::HazardType::BLIZZARD, 0.5f, false}},
	};

	std::optional<TileHazardProfile> HazardDB::getTileHazardProfile(types::TileType tileType) {
		static std::optional<TileHazardProfile> empty;
		auto profile = tileHazardProfiles.find(tileType);
		if (profile != tileHazardProfiles.end())
			return profile->second;
		return empty;
	}

} // namespace df
