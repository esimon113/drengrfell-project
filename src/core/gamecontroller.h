#pragma once

#include <cstddef>
#include <random>
#include <vector>

#include "gamestate.h"
#include "road.h"
#include "systems/questsSystem.h"




namespace df {

	/*
	 * The GameController manages the high-level game flwo / mechanics.
	 * Component specific responsibilities are handled by the corresponding classes (Tile, Hero, ...).
	 */
	class GameController {
	  public:
		GameController() = default;
		~GameController() = default;
		explicit GameController(GameState& state)
			: gameState(state), rng(std::random_device{}()) {
				m_questsSystem = std::make_unique<QuestsSystem>();
			};

		GameState& getState() { return this->gameState; }
		const GameState& getState() const { return this->gameState; }

		Player* getCurrentPlayer();
		const Player* getCurrentPlayer() const;

        void startTurn(Registry& registry);
		void endTurn(Registry& registry);

		void applyHazard(Entity hero, Registry& registry, glm::vec2 destination);
		void updateHazards(Registry& registry);
		void showHazards(Registry& registry);
		void payForHazard(Registry& registry);

        void giveResourcesTo(Player& player);

		bool moveHeroToTile(size_t playerId, size_t targetTileId);

		bool canBuildSettlement(size_t playerId, size_t vertexId) const;
		bool buildSettlement(size_t playerId, size_t vertexId, const std::vector<int>& buildingCost);

		bool canBuildRoad(size_t playerId, size_t edgeId) const;
		bool buildRoad(size_t playerId, size_t edgeId, RoadLevel level, const std::vector<int>& buildingCost);

		QuestsSystem* getQuestsSystem() const { return m_questsSystem.get(); }
		void claimQuestReward(int questId);

	  private:
		GameState& gameState;
		std::mt19937 rng;
		std::unique_ptr<QuestsSystem> m_questsSystem;

		Player* getPlayerbyId(size_t playerId);
		const Player* getPlayerById(size_t playerId) const;

		void resetHeroMovement(Player& player);
		void exploreTile(Player& player, size_t tileId);

		bool doesVertexHaveNeighborSettlements(size_t vertexId) const;
		bool doesEdgeConnectToPlayer(size_t playerId, size_t edgeId) const;

		// returns ids of tiles touching the settlement -> TODO: move to settlement class
		std::vector<size_t> getSettlementTiles(const Settlement& settlement) const;

		bool hasEnoughResources(Player& player, const std::vector<int>& buildingCost);
		void chargeResourceCost(Player& player, const std::vector<int>& buildingCost);

		const Road* findRoadById(size_t roadId) const;
		const Settlement* findSettlementById(size_t settlementId) const;
	};

} // namespace df
