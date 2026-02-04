#include "tile.h"
#include "fmt/base.h"
#include "types.h"

#include <random>

namespace df {
	const json Tile::serialize() const {
		json j;

		j["id"] = id;
		// static_cast turns the enums into ints so they are json-compatible
		j["type"] = static_cast<int>(type);
		j["basePotency"] = static_cast<int>(basePotency);
		j["rangeFactor"] = rangeFactor;

		if (buildingId.has_value())
			j["buildingId"] = buildingId.value();
		else
			j["buildingId"] = nullptr;

		j["visibleForPlayers"] = visibleForPlayers;

		return j;
	}


	void Tile::deserialize(const json& j) {
		size_t newId = j["id"];
		this->setId(newId);

		int typeInt = j["type"].get<int>();
		types::TileType newType = static_cast<types::TileType>(typeInt);
		this->setType(newType);

		int potencyInt = j["basePotency"].get<int>();
		types::TilePotency newPotency = static_cast<types::TilePotency>(potencyInt);
		this->setPotency(newPotency);

		float newRange = j["rangeFactor"];
		this->setRangeFactor(newRange);

		if (!j["id"].is_null()) {
			std::optional<size_t> newBuildingId = j["buildingId"];
			this->setBuildingId(newBuildingId);
		} else {
			buildingId.reset();
		}

		std::vector<size_t> newVisibleForPlayers = j["visibleForPlayers"];
		this->setVisibleForPlayers(newVisibleForPlayers);
	}


	float Tile::getPotencyProbability(types::TilePotency currPotency) const {
		switch (currPotency) { // TODO: make probabilities configurable
		case types::TilePotency::LOW:
			return 0.2f;
		case types::TilePotency::MEDIUMLOW:
			return 0.35f;
		case types::TilePotency::MEDIUM:
			return 0.5f;
		case types::TilePotency::MEDIUMHIGH:
			return 0.7f;
		case types::TilePotency::HIGH:
			return 0.9f;
		default:
			return 0.0f;
		}
	}

	double Tile::getMovementCost() const noexcept {
		double cost;

		switch (type) {
		case types::TileType::GRASS:
			cost = 1.0;
			break;
		case types::TileType::FIELD:
			cost = 1.0;
			break;
		case types::TileType::FOREST:
			cost = 1.5;
			break;
		case types::TileType::CLAY:
			cost = 1.0;
			break;
		case types::TileType::ICE:
			cost = 2.0;
			break;
		case types::TileType::MOUNTAIN:
			cost = 2.0;
			break;
		case types::TileType::WATER:
			cost =50.0;
			break;
		default:
			cost = 1.0;
		}
		return cost;
	}


	bool Tile::isResourceTile() const {
		switch (this->type) {
		case types::TileType::EMPTY:
		case types::TileType::WATER: // TODO: discuss: maybe use water to get resource fish?!
		case types::TileType::ICE:
		case types::TileType::COUNT:
			return false;
		default:
			return true;
		}
	}


	bool Tile::givesResourceThisTurn(std::mt19937& rng) const {
		if (!this->isResourceTile()) {
			return false;
		}

		std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
		auto dist = distribution(rng);
		auto currentPotency = this->getEffectivePotency(); 
		auto prob = this->getPotencyProbability(currentPotency);
		fmt::println("Dice Result: {}, resource probability: {}", dist, prob);
		return dist <= prob;
	}

	void Tile::initializeHazardProfile() {
		hazardProfile = HazardDB::getTileHazardProfile(type);
	}

	types::TilePotency Tile::getEffectivePotency() const {
		int effective = static_cast<int>(basePotency) + weatherModifier;
		
		if (effective < 1) effective = 1;
		if (effective > 5) effective = 5;
		
		return static_cast<types::TilePotency>(effective);
	}

	void Tile::updateEffect(types::WeatherType weather) noexcept {
		weatherModifier = 0; 

		if (weather == types::WeatherType::SNOW) {
			weatherModifier = -1; 
		} 
		else if (weather == types::WeatherType::RAIN) {
			if (type == types::TileType::FIELD || type == types::TileType::GRASS || type == types::TileType::FOREST)
				weatherModifier = 1;
			else if (type == types::TileType::CLAY || type == types::TileType::MOUNTAIN)
				weatherModifier = -1;
		}
		else if (weather == types::WeatherType::SUNNY) {
			if (type == types::TileType::MOUNTAIN || type == types::TileType::CLAY)
				weatherModifier = 1;
		}
	}

	std::string Tile::getPotencyModifierLabel(types::WeatherType weather) const {
		if (weather == types::WeatherType::SUNNY) {
			if (weatherModifier > 0) return " [BOOSTED by SUN]";
		}

		if (weather == types::WeatherType::RAIN) {
			if (weatherModifier > 0) return " [BOOSTED by RAIN]";
			if (weatherModifier < 0) return " [DAMPENED by RAIN]";
		}
		
		if (weather == types::WeatherType::SNOW) {
			if (weatherModifier < 0) return " [DAMPENED by SNOW]";
		}

		return "";
	}

} // namespace df
