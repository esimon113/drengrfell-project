#pragma once

#include <cstddef>
#include <memory>
#include <random>
#include <vector>

#include "gamestate.h"
#include "road.h"
#include "systems/questsSystem.h"


namespace df {

	class GameController {
	  public:
		explicit GameController(GameState& state)
			: gameState(state), rng(std::random_device{}()), m_questsSystem(std::make_unique<QuestsSystem>()) {}

		GameState& getState() { return this->gameState; }
		const GameState& getState() const { return this->gameState; }

		Player* getCurrentPlayer();
		const Player* getCurrentPlayer() const;

		void startTurn();
		void endTurn();

		void applyHazard(size_t playerId, size_t tileId);
		void rollWeather();
		void updateHazards();
		void showHazards();
		void payForHazard();

		void giveResourcesTo(Player& player);

		bool canMoveHeroToTile(size_t playerId, size_t targetTileId) const;
		bool moveHeroToTile(size_t playerId, size_t targetTileId);
		bool canAfford(size_t playerId, const std::vector<int>& cost) const;

		bool canBuildSettlement(size_t playerId, size_t vertexId) const;
		bool buildSettlement(size_t playerId, size_t vertexId, const std::vector<int>& buildingCost);

		bool canBuildRoad(size_t playerId, size_t edgeId) const;
		bool buildRoad(size_t playerId, size_t edgeId, RoadLevel level, const std::vector<int>& buildingCost);

		bool canBuildProductivityBuilding(size_t playerId, size_t tileId, types::TileType tileType) const;
		bool buildProductivityBuilding(size_t playerId, size_t tileId, types::TileType tileType, const std::vector<int>& buildingCost);

		bool canUpgradeSettlement(size_t playerId, size_t settlementId, types::SettlementType targetType) const;
		bool upgradeSettlement(size_t playerId, size_t settlementId, types::SettlementType targetType, const std::vector<int>& buildingCost);

		QuestsSystem* getQuestsSystem() const { return m_questsSystem.get(); }
		void claimQuestReward(int questId);

		Player* getPlayerbyId(size_t playerId);
		const Player* getPlayerById(size_t playerId) const;

		int getCountCastles(size_t playerId);

	  private:
		GameState& gameState;
		std::mt19937 rng;
		std::unique_ptr<QuestsSystem> m_questsSystem;

		void resetHeroMovement(Player& player);
		void exploreTile(Player& player, size_t tileId);

		bool doesVertexHaveNeighborSettlements(size_t vertexId) const;
		bool doesEdgeConnectToPlayer(size_t playerId, size_t edgeId) const;

		std::vector<size_t> getSettlementTiles(const Settlement& settlement) const;

		bool hasEnoughResources(Player& player, const std::vector<int>& buildingCost);
		void chargeResourceCost(Player& player, const std::vector<int>& buildingCost);

		const Road* findRoadById(size_t roadId) const;
		const Settlement* findSettlementById(size_t settlementId) const;
		const ProductivityBuilding* findProductivityBuildingById(size_t buildingId) const;
		const ProductivityBuilding* findProductivityBuildingByTileId(size_t tileId) const;
	};

} // namespace df
