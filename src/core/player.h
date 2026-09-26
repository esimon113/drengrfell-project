#pragma once


#include <map>
#include <memory>
#include <string>
#include <vector>

#include "settlement.h"
#include "tile.h"
#include "types.h"
#include "graph.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include "hero.h"
#include "road.h"
#include <optional>



namespace df {
	class Graph;
	class Player {
	  public:
		struct ActiveHazard {
			types::HazardType type{types::HazardType::NONE};
			int turnsLeft{0};
		};

	  private:

		size_t playerId;
		int heroPoints;
		std::string name;
		size_t tutorialStep{0};
		std::vector<size_t> settlementIds;
		std::map<types::TileType, int> resources;
		std::shared_ptr<Hero> heroReference;
		std::vector<size_t> roadIds;
		std::vector<size_t> productivityBuildingIds;
		std::vector<size_t> exploredTileIds;
		std::optional<ActiveHazard> activeHazard;


	  public:
		Player();
		Player(size_t id);
		size_t getId() const;

		int getHeroPoints() const;
		void addHeroPoints(int);
		void setHeroPoints(int);

		const std::string& getName() const;
		void setName(const std::string& newName);

		size_t getTutorialStep() const;
		void setTutorialStep(size_t step);

		// Changed to return IDs
		const std::vector<size_t>& getSettlementIds() const;
		void addSettlement(size_t settlementId);
		void removeSettlement(size_t settlementId);

		void addResources(types::TileType, int);
		void removeResources(types::TileType, int);
		int getResources(types::TileType type) const;
		bool hasResources(const std::map<types::TileType, int>&);
		const std::map<types::TileType, int>& getResources() const;

		void setHero(std::shared_ptr<Hero> hero);
		std::shared_ptr<Hero> getHero() const;

		void addRoad(size_t roadId);
		const std::vector<size_t>& getRoadIds() const;
		int getRoadCount() const;

		void addProductivityBuilding(size_t buildingId);
		const std::vector<size_t>& getProductivityBuildingIds() const;

		bool exploreTile(size_t tileId);
		bool isTileExplored(size_t tileId) const;
		const std::vector<size_t>& getExploredTileIds() const;
		void forgetExploredTiles();
		int retExploredCount(const Graph& map) const;

		size_t getPlayerId() const;
		void setPlayerId(size_t newPlayerId);

		const json serialize() const;

		void deserialize(const json& j);

		void reset();

		bool hasActiveHazard() const;
		const std::optional<ActiveHazard>& getActiveHazard() const;
		void setActiveHazard(ActiveHazard hazard);
		void clearActiveHazard();
	};
} // namespace df
