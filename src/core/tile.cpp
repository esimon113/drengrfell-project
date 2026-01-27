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
		j["potency"] = static_cast<int>(potency);
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

		int potencyInt = j["potency"].get<int>();
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
			return 0.3f;
		case types::TilePotency::MEDIUM:
			return 0.5f;
		case types::TilePotency::HIGH:
			return 0.9f;
		default:
			return 0.0f;
		}
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
		auto prob = this->getPotencyProbability(this->potency);
		fmt::println("Dice Result: {}, resource probability: {}", dist, prob);
		return dist <= prob;
	}

	void Tile::initializeHazardProfile() {
		hazardProfile = HazardDB::getTileHazardProfile(type);
	}

	void Tile::updateEffect(types::WeatherType weather) noexcept {
		switch (this->type) {
			case types::TileType::FIELD:
				if (weather == types::WeatherType::RAIN)       potency = types::TilePotency::HIGH;
				else if (weather == types::WeatherType::SNOW)  potency = types::TilePotency::LOW;
				else                                    potency = types::TilePotency::MEDIUM; // SUNNY
				break;

			case types::TileType::CLAY:
				if (weather == types::WeatherType::SUNNY)      potency = types::TilePotency::MEDIUM;
				else if (weather == types::WeatherType::RAIN)  potency = types::TilePotency::LOW;
				else                                    potency = types::TilePotency::LOW;    // SNOW
				break;

			case types::TileType::FOREST:
				if (weather == types::WeatherType::SUNNY)      potency = types::TilePotency::MEDIUM;
				else if (weather == types::WeatherType::RAIN)  potency = types::TilePotency::MEDIUM;
				else                                    potency = types::TilePotency::LOW;    // SNOW
				break;

			case types::TileType::MOUNTAIN:
				if (weather == types::WeatherType::SUNNY)      potency = types::TilePotency::MEDIUM;
				else if (weather == types::WeatherType::RAIN)  potency = types::TilePotency::LOW;
				else                                    potency = types::TilePotency::LOW;    // SNOW
				break;

			case types::TileType::GRASS:
				if (weather == types::WeatherType::RAIN)       potency = types::TilePotency::HIGH;
				else if (weather == types::WeatherType::SNOW)  potency = types::TilePotency::LOW;
				else                                    potency = types::TilePotency::MEDIUM; // SUNNY
				break; 

			default:
				potency = types::TilePotency::MEDIUM;
				break;
		}
	}
} // namespace df
