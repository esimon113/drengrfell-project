#include "edge.h"
#include "fmt/base.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <optional>
#include <stdexcept>
#include <unordered_set>

#include "gamecontroller.h"
#include "hazards.h"
#include "hero.h"
#include "player.h"
#include "road.h"
#include "tile.h"
#include "types.h"
#include "vertex.h"
#include "utils/worldNodeMapper.h"


namespace df {

	Player* GameController::getCurrentPlayer() { return this->getPlayerbyId(this->gameState.getCurrentPlayerId()); }
	const Player* GameController::getCurrentPlayer() const { return this->getPlayerById(this->gameState.getCurrentPlayerId()); }


	Player* GameController::getPlayerbyId(size_t playerId) { return this->gameState.getPlayer(playerId); }
	const Player* GameController::getPlayerById(size_t playerId) const { return this->gameState.getPlayer(playerId); }


	void GameController::startTurn() {
		Player* player = this->getCurrentPlayer();
		if (!player) {
			fmt::println("Current Player does not exist!");
			return;
		}

		this->giveResourcesTo(*player);
		this->resetHeroMovement(*player);
		this->showHazards();
	}


	void GameController::endTurn() {
		const size_t playerCount = this->gameState.getPlayerCount();
		if (playerCount == 0) {
			return;
		}

		this->updateHazards();

		size_t nextPlayerId = (this->gameState.getCurrentPlayerId() + 1) % playerCount;
		this->gameState.setCurrentPlayerId(nextPlayerId);
		this->gameState.setTurnCount(this->gameState.getTurnCount() + 1);

		if (this->m_questsSystem) {
			this->m_questsSystem->updateProgress(df::types::QuestGoalType::ROUNDS, 1);
		}

		if (nextPlayerId == 0) {
			this->gameState.setRoundNumber(this->gameState.getRoundNumber() + 1);
		}
	}

	void GameController::rollWeather() {
		const types::WeatherType previous = this->gameState.getWeather();
		static constexpr float transition[3][3] = {
			{0.70f, 0.20f, 0.10f},
			{0.25f, 0.60f, 0.15f},
			{0.20f, 0.20f, 0.60f},
		};

		std::uniform_real_distribution<float> dist(0.0f, 1.0f);
		const float roll = dist(this->rng);
		const int row = static_cast<int>(previous);
		float cumulative = 0.0f;
		types::WeatherType next = previous;
		for (int col = 0; col < 3; ++col) {
			cumulative += transition[row][col];
			if (roll <= cumulative) {
				next = static_cast<types::WeatherType>(col);
				break;
			}
		}

		float intensity = this->gameState.getWeatherIntensity();
		if (next == types::WeatherType::SUNNY) {
			intensity = 0.0f;
		} else if (next == types::WeatherType::RAIN) {
			intensity = previous == types::WeatherType::RAIN ? intensity - 0.2f : -0.6f;
		} else if (next == types::WeatherType::SNOW) {
			intensity = previous == types::WeatherType::SNOW ? intensity + 0.2f : 0.6f;
		}

		this->gameState.setWeather(next);
		this->gameState.setWeatherIntensity(intensity);
		if (next != previous) {
			for (const auto& tile : this->gameState.getMap().getTiles()) {
				if (tile) {
					tile->updateEffect(next);
				}
			}
		}
	}

	void GameController::applyHazard(size_t playerId, size_t tileId) {
		Player* player = this->getPlayerbyId(playerId);
		if (!player || player->hasActiveHazard()) {
			return;
		}

		TileHandle tile = this->gameState.getMap().getTile(tileId);
		if (!tile) {
			return;
		}

		const auto& profileOpt = tile->getHazardProfile();
		if (!profileOpt) {
			return;
		}

		const auto& profile = *profileOpt;
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);
		if (dist(rng) > profile.probability) {
			return;
		}

		const auto& def = HazardDB::getDefinition(profile.hazardType);
		player->setActiveHazard({profile.hazardType, def.defaultRoundDuration});
	}

	void GameController::updateHazards() {
		for (Player& player : this->gameState.getPlayers()) {
			if (!player.hasActiveHazard()) {
				continue;
			}
			auto hazard = *player.getActiveHazard();
			hazard.turnsLeft--;
			if (hazard.turnsLeft <= 0) {
				player.clearActiveHazard();
			} else {
				player.setActiveHazard(hazard);
			}
		}
	}

	void GameController::showHazards() {
		Player* player = this->getCurrentPlayer();
		if (!player || !player->hasActiveHazard()) {
			return;
		}
		if (player->getActiveHazard()->turnsLeft <= 0) {
			player->clearActiveHazard();
		}
	}

	void GameController::payForHazard() {
		Player* player = this->getCurrentPlayer();
		if (!player || !player->hasActiveHazard()) {
			return;
		}

		const auto hazard = *player->getActiveHazard();
		const auto& hazardDefinition = HazardDB::getDefinition(hazard.type);
		const int cost = hazard.turnsLeft * hazardDefinition.skipCost;
		if (player->getResources(hazardDefinition.skipRessource) < cost) {
			return;
		}
		player->removeResources(hazardDefinition.skipRessource, cost);
		player->clearActiveHazard();
	}


	void GameController::giveResourcesTo(Player& player) {
		// resources are given to the player based on the settlements they have

		for (size_t settlementId : player.getSettlementIds()) {
			const Settlement* settlement = this->findSettlementById(settlementId);
			if (!settlement) {
				continue;
			}
			fmt::println("Try getting resources for settlement with id: {}", settlementId);

			const auto tileIds = this->getSettlementTiles(*settlement);
			for (size_t tileId : tileIds) {
				const TileHandle tile = this->gameState.getMap().getTile(tileId);
				fmt::println("Get TileId {}, tile has type {} and potency {}", tileId, std::string(types::tileTypeToString(tile->getType())), types::potencyToString(tile->getPotency()));
				if (tile->givesResourceThisTurn(this->rng)) {
					int resourceAmount = 1;
					if (tile->hasBuilding() && tile->getBuildingId().has_value()) {
						if (tile->getBuildingId().value() == player.getId()) {
							resourceAmount = 2;
						}
					}
					player.addResources(tile->getType(), resourceAmount); // TODO: make amount configurable -> i.e. in settlers of catan a town gives 2 resources

					// std::string type = types::tileTypeToString(tile->getType());
					// std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) { return std::tolower(c); });
					auto goalType = types::tileToQuestGoal(tile->getType());
					if (goalType != types::QuestGoalType::NONE) {
						this->m_questsSystem->updateProgress(goalType, 1);
					}
				}
			}
		}

		// Also grant resources from the tile the hero is currently on.
		if (player.getHero()) {
			const size_t heroTileId = player.getHero()->getTileID();
			const TileHandle heroTile = this->gameState.getMap().getTile(heroTileId);
			if (heroTile && heroTile->givesResourceThisTurn(this->rng)) {
				int resourceAmount = 1;
				if (heroTile->hasBuilding() && heroTile->getBuildingId().has_value()) {
					if (heroTile->getBuildingId().value() == player.getId()) {
						resourceAmount = 2;
					}
				}
				player.addResources(heroTile->getType(), resourceAmount);
				auto goalType = types::tileToQuestGoal(heroTile->getType());
				if (goalType != types::QuestGoalType::NONE && this->m_questsSystem) {
					this->m_questsSystem->updateProgress(goalType, 1);
				}
			}
		}
	}


	void GameController::resetHeroMovement(Player& player) {
		if (player.getHero()) {
			player.getHero()->setMovedThisTurn(false);
		}
	}


	void GameController::exploreTile(Player& player, size_t tileId) {
		Graph& map = this->gameState.getMap();

		try {
			TileHandle tile = map.getTile(tileId);

			if (!player.isTileExplored(tileId)) {
				tile->addVisibleForPlayers(player.getId());
				player.exploreTile(tileId);
				if (this->m_questsSystem && tile->getType() != types::TileType::WATER) {
					this->m_questsSystem->updateProgress(types::QuestGoalType::DISCOVER, 1);
				}
				if (this->m_questsSystem && tile->getType() == types::TileType::ICE) {
					this->m_questsSystem->updateProgress(types::QuestGoalType::ICE, 1);
				}
			}
		} catch (const std::exception&) {
		} // invalid tile -> ignore
	}


	bool GameController::canMoveHeroToTile(size_t playerId, size_t targetTileId) const {
		const Player* player = this->getPlayerById(playerId);
		if (!player) {
			return false;
		}

		const std::shared_ptr<Hero> hero = player->getHero();
		if (!hero || hero->hasMovedThisTurn()) {
			return false;
		}

		const size_t distance = this->gameState.getMap().getTileStepDistance(hero->getTileID(), targetTileId);
		if (distance == SIZE_MAX) {
			return false;
		}

		return static_cast<int>(distance) <= hero->getBaseRange();
	}


	bool GameController::moveHeroToTile(size_t playerId, size_t targetTileId) {
		if (!this->canMoveHeroToTile(playerId, targetTileId)) {
			return false;
		}

		Player* player = this->getPlayerbyId(playerId);
		std::shared_ptr<Hero> hero = player->getHero();
		const size_t startTile = hero->getTileID();
		std::vector<size_t> path = this->gameState.getMap().dijkstraPath(startTile, targetTileId, player);
		const int range = hero->getBaseRange();
		const size_t maxTiles = static_cast<size_t>(range > 0 ? range : 0) + 1;
		if (path.size() > maxTiles) {
			path.resize(maxTiles);
		}
		if (path.empty()) {
			this->exploreTile(*player, targetTileId);
			hero->setTileID(targetTileId);
		} else {
			for (size_t tileId : path) {
				this->exploreTile(*player, tileId);
			}
			hero->setTileID(path.back());
		}
		hero->setMovedThisTurn(true);

		return true;
	}


	bool GameController::canBuildSettlement(size_t playerId, size_t vertexId) const {
		const Graph& map = this->gameState.getMap();
		const Player* player = this->gameState.getPlayer(playerId);
		if (!player || !player->getHero()) {
			return false;
		}
		try {
			// Find vertex by ID (not index)
			VertexHandle vertex = map.findVertexById(vertexId);
			if (!vertex) {
				fmt::println("[GameController] canBuildSettlement: vertex {} not found", vertexId);
				return false;
			}

			// Only check if vertex already has a settlement
			if (vertex->hasSettlement()) {
				fmt::println("[GameController] canBuildSettlement: vertex {} already has settlement {}", vertexId, vertex->getSettlementId().value_or(SIZE_MAX));
				return false;
			}

			// Also check that no adjacent vertices have settlements (basic rule)
			if (this->doesVertexHaveNeighborSettlements(vertexId)) {
				fmt::println("[GameController] canBuildSettlement: neighbour settlement detected for vertex {}", vertexId);
				return false;
			}

			// Make sure that the vertex is not surrounded by water -> TODO: make this check more robust
			const auto vertexTiles = map.getVertexTiles(vertex);
			if (!vertexTiles) {
				return false;
			}
			bool hasNonWaterTile = false;
			for (const auto& t : *vertexTiles) {
				if (t && t->getType() != types::TileType::WATER) {
					hasNonWaterTile = true;
					break;
				}
			}
			if (!hasNonWaterTile) { // no non-water tile
				return false;
			}

			// check if EITHER vertex is adjacent to hero-tile, OR adjacent to road of current player
			// const auto player = gameState.getPlayer(playerId);
			// if (!player) {
			// 	fmt::println("No player with id {}", playerId);
			// 	return false;
			// }

			const auto vertexEdges = map.getVertexEdges(vertex);
			if (!vertexEdges) {
				fmt::println("No vertex with id {}", vertexId);
				return false;
			}


			const auto heroTileId = player->getHero()->getTileID();
			fmt::println("[GameController] Hero tile id {}", heroTileId);
			fmt::println("[GameController] Check Hero tile {}", heroTileId);
			for (const auto& t : *vertexTiles) {
				if (t && t->getId() == heroTileId) { // vertex is adjacent to hero tile
					fmt::println("[GameController] canBuildSettlement: vertex {} is a valid placement", vertexId);
					return true; // player can always build a settlement adjacent to hero-tile
				}
			}

			// // Use hero of player:
			// const auto hero = player->getHero();
			// if (!hero) {
			// 	fmt::println("No hero for player with id {}", playerId);
			// } else {
			// 	const auto tileId = hero->getTileID();
			// 	fmt::println("[GameController] canBuildSettlement: hero tile ID: {}", tileId);
			// 	const auto tile = map.getTile(tileId);
			// 	if (tile) {
			// 		const auto tileVertices = map.getTileVertices(tile);
			// 		if (tileVertices) {
			// 			for (const auto& v : *tileVertices) {
			// 				if (v && v->getId() == vertexId) { // vertex is adjacent to hero tile
			// 					fmt::println("[GameController] canBuildSettlement: vertex {} is a valid placement", vertexId);
			// 					return true; // player can always build a settlement adjacent to hero-tile
			// 				}
			// 			}
			// 		}
			// 	}
			// }

			const auto roads = gameState.getRoads();

			for (const auto& e : *vertexEdges) {
				if (!e) {
					continue;
				}
				const auto localRoadId = e->getRoadId();
				if (!localRoadId) {
					continue;
				}

				auto it = std::find_if(roads.begin(), roads.end(), [localRoadId](const auto& r) {
					return r->getId() == localRoadId; // && r->getPlayerId() == playerId;
				});

				if (it != roads.end()) {
					fmt::println("[GameController] canBuildSettlement: vertex {} is a valid placement", vertexId);
					return true;
				}
			}

			fmt::println("[GameController] canBuildSettlement: vertex {} not adjacent to hero and no connected road", vertexId);
			return false;
		} catch (const std::exception& e) {
			fmt::println("Error in settlement building validation: {}", e.what());
		}
		return false;
	}


	bool GameController::buildSettlement(size_t playerId, size_t vertexId, const std::vector<int>& buildingCost) {
		if (!this->canBuildSettlement(playerId, vertexId)) {
			fmt::println("[GameController] buildSettlement failed: canBuildSettlement returned false");
			return false;
		}

		Player* player = this->getPlayerbyId(playerId);
		if (!player) {
			fmt::println("[GameController] buildSettlement failed: player {} not found", playerId);
			return false;
		}
		if (!this->hasEnoughResources(*player, buildingCost)) {
			fmt::println("[GameController] buildSettlement failed: player {} does not have enough resources", playerId);
			return false;
		}

		Graph& map = this->gameState.getMap();

		try {
			fmt::println("[GameController] buildSettlement: requested at vertex {}", vertexId);
			// Find vertex by ID (not index) - vertexId is the ID stored in the Vertex object
			VertexHandle vertex = map.findVertexById(vertexId);

			if (!vertex) {
				fmt::println("[GameController] buildSettlement failed: vertex {} not found in map", vertexId);
				return false; // Vertex with this ID not found
			}

			// Double-check vertex doesn't already have a settlement (race condition protection)
			if (vertex->hasSettlement()) {
				fmt::println("[GameController] buildSettlement failed: vertex {} already has a settlement", vertexId);
				return false;
			}

			size_t newSettlementId = 0;
			const auto& existingSettlements = this->gameState.getSettlements();
			if (!existingSettlements.empty()) {
				size_t maxId = 0;
				for (const auto& s : existingSettlements) {
					if (s && s->getId() > maxId) {
						maxId = s->getId();
					}
				}
				newSettlementId = maxId + 1;
			}

			// TODO: rethink ownership of settlement
			auto newSettlement = std::make_shared<Settlement>(newSettlementId, playerId, vertexId, buildingCost);

			vertex->setSettlementId(newSettlementId);
			this->gameState.addSettlement(newSettlement);
			player->addSettlement(newSettlement->getId());
            fmt::println("[GameController] Player {} awarded 1 point for new settlement", playerId);

			// this->chargeResourceCost(*player, newSettlement->getBuildingCost());
			this->chargeResourceCost(*player, buildingCost);

			m_questsSystem->updateProgress(types::QuestGoalType::SETTLEMENT, 1);


			fmt::println("[GameController] buildSettlement succeeded: settlement {} built at vertex {} for player {}", newSettlementId, vertexId, playerId);

			return true;

		} catch (const std::exception& e) {
			fmt::println("[GameController] buildSettlement failed: exception - {}", e.what());
			return false;
		}
	}


	// TODO: validate this in edge class
	bool GameController::canBuildRoad(size_t playerId, size_t edgeId) const {
		const Graph& map = this->gameState.getMap();

		try {
			// Find edge by ID (not index)
			EdgeHandle edge = map.findEdgeById(edgeId);
			if (!edge) {
				fmt::println("No edge with id {}", edgeId);
				return false;
			}

			// Only check if edge already has a road
			if (edge->hasRoad()) {
				fmt::println("Edge already has a road", edgeId);
				return false;
			}

			// roads can only be build if:
			// 1. they are adjacent to a settlement of the current player
			// 2. they are adjacent to a road of the current player

			const auto edgeVertices = map.getEdgeVertices(edge);
			if (!edgeVertices) {
				fmt::println("No vertices for edge");
				return false;
			}

			const auto settlements = this->gameState.getSettlements();
			const auto roads = this->gameState.getRoads();
			for (const auto& v : *edgeVertices) {
				if (!v) {
					continue;
				}
				// check for adjacent player settlements:
				if (const auto sid = v->getSettlementId(); sid) {
					const auto it = std::find_if(settlements.begin(), settlements.end(), [&](const auto& s) {
						return s->getId() == sid && s->getPlayerId() == playerId;
					});

					if (it != settlements.end()) {
						return true;
					}
				}

				// check for adjacent roads
				const auto vertexEdges = map.getVertexEdges(v);
				if (!vertexEdges) {
					fmt::println("No edges for vertex");
					return false;
				}

				for (const auto& neighbourEdge : *vertexEdges) {
					if (!neighbourEdge) {
						continue;
					}
					if (neighbourEdge->getId() == edgeId) {
						continue; // ignore self
					}

					const auto it = std::find_if(roads.begin(), roads.end(), [playerId, neighbourEdge](const auto& r) {
						return r->getEdgeId() == neighbourEdge->getId() && r->getPlayerId() == playerId;
					});

					if (it != roads.end()) {
						return true;
					}
				}
			}

			return false;
		} catch (const std::exception&) {
			return false;
		}
	}


	bool GameController::buildRoad(size_t playerId, size_t edgeId, RoadLevel level, const std::vector<int>& buildingCost) {
		if (!this->canBuildRoad(playerId, edgeId)) {
			fmt::println("[GameController] buildRoad failed: canBuildRoad returned false");
			return false;
		}

		Player* player = this->getPlayerbyId(playerId);
		if (!player) {
			fmt::println("[GameController] buildRoad failed: player {} not found", playerId);
			return false;
		}

		if (!this->hasEnoughResources(*player, buildingCost)) {
			fmt::println("[GameController] buildRoad failed: player {} does not have enough resources", playerId);
			return false;
		}

		Graph& map = this->gameState.getMap();
		try {
			// Find edge by ID (not index)
			EdgeHandle edge = map.findEdgeById(edgeId);
			if (!edge) {
				fmt::println("[GameController] buildRoad failed: edge {} not found in map", edgeId);
				return false;
			}

			// Double-check edge doesn't already have a road
			if (edge->hasRoad()) {
				fmt::println("[GameController] buildRoad failed: edge {} already has a road", edgeId);
				return false;
			}

			// CRITICAL FIX: Check ALL edges that share the same physical location (same two vertices)
			// This prevents building multiple roads on the same physical edge due to duplicate edge IDs
			// This is a safeguard check - if it fails for any reason, we still allow building
			try {
				const auto verticesOpt = map.getEdgeVertices(edge);
				if (verticesOpt) {
					// Get the two vertex IDs that this edge connects
					std::unordered_set<size_t> edgeVertexIds;
					for (const auto& vertex : *verticesOpt) {
						if (vertex && vertex->getId() != SIZE_MAX) {
							edgeVertexIds.insert(vertex->getId());
						}
					}

					// Only check for duplicates if this edge connects exactly two valid vertices (shared edge)
					// If it doesn't have 2 vertices, we skip the duplicate check and allow building
					if (edgeVertexIds.size() == 2) {
						// Only check edges that already have roads (optimization and safety)
						// This allows the first road to be built without any checks
						for (size_t i = 0; i < map.getEdgeCount(); ++i) {
							EdgeHandle otherEdge = map.getEdge(i);
							if (!otherEdge || otherEdge->getId() == edgeId || !otherEdge->hasRoad()) {
								continue; // Skip if no road - no conflict possible
							}

							const auto otherVerticesOpt = map.getEdgeVertices(otherEdge);
							if (!otherVerticesOpt)
								continue;

							// Check if this edge connects the same two vertices
							std::unordered_set<size_t> otherVertexIds;
							for (const auto& vertex : *otherVerticesOpt) {
								if (vertex && vertex->getId() != SIZE_MAX) {
									otherVertexIds.insert(vertex->getId());
								}
							}

							// If the vertex sets match exactly (same two vertices), they're at the same physical location
							if (otherVertexIds.size() == 2 && edgeVertexIds == otherVertexIds) {
								fmt::println("[GameController] buildRoad failed: edge {} connects same vertices as edge {} which has a road",
											 edgeId, otherEdge->getId());
								return false;
							}
						}
					}
					// If edgeVertexIds.size() != 2, we skip the duplicate check and allow building
				}
				// If verticesOpt is nullopt, we also allow building (edge case)
			} catch (const std::exception& e) {
				// If the duplicate check fails for any reason, we still allow building
				// This is a safeguard check and shouldn't block legitimate road building
				fmt::println("[GameController] buildRoad: duplicate check failed for edge {}: {}, allowing building", edgeId, e.what());
			}

			// generate unique road id -> use the max existing id + 1, or 0 if no roads exist
			size_t roadId = 0;
			const auto& existingRoads = this->gameState.getRoads();
			if (!existingRoads.empty()) {
				size_t maxId = 0;
				for (const auto& r : existingRoads)
					if (r && r->getId() > maxId)
						maxId = r->getId();
				roadId = maxId + 1;
			}

			auto road = std::make_shared<Road>(roadId, playerId, edgeId, level, buildingCost);

			edge->setRoadId(roadId);
			this->gameState.addRoad(road);
			player->addRoad(road->getId());

			this->chargeResourceCost(*player, buildingCost);

			m_questsSystem->updateProgress(types::QuestGoalType::ROAD, 1);

			fmt::println("[GameController] buildRoad succeeded: road {} built at edge {} for player {}", roadId, edgeId, playerId);

			return true;

		} catch (const std::exception& e) {
			fmt::println("[GameController] buildRoad failed: exception - {}", e.what());
			return false;
		}
	}

	bool GameController::canBuildProductivityBuilding(size_t playerId, size_t tileId, types::TileType tileType) const {
		const Graph& map = this->gameState.getMap();
		const TileHandle tile = map.getTile(tileId);
		if (!tile) {
			return false;
		}

		if (tile->hasBuilding()) {
			return false;
		}

		if (tile->getType() != tileType) {
			return false;
		}

		const auto settlements = this->gameState.getSettlements();
		for (const auto& settlement : settlements) {
			if (!settlement || settlement->getPlayerId() != playerId) {
				continue;
			}
			const auto adjacentTiles = this->getSettlementTiles(*settlement);
			if (std::find(adjacentTiles.begin(), adjacentTiles.end(), tileId) != adjacentTiles.end()) {
				return true;
			}
		}

		return false;
	}

	bool GameController::buildProductivityBuilding(size_t playerId, size_t tileId, types::TileType tileType, const std::vector<int>& buildingCost) {
		Player* player = this->getPlayerbyId(playerId);
		if (!player) {
			fmt::println("[GameController] buildProductivityBuilding failed: player {} not found", playerId);
			return false;
		}
		if (!this->hasEnoughResources(*player, buildingCost)) {
			return false;
		}
		if (!this->canBuildProductivityBuilding(playerId, tileId, tileType)) {
			return false;
		}

		Graph& map = this->gameState.getMap();
		TileHandle tile = map.getTile(tileId);
		if (!tile || tile->hasBuilding()) {
			return false;
		}

		size_t newBuildingId = 0;
		const auto& existingBuildings = this->gameState.getProductivityBuildings();
		if (!existingBuildings.empty()) {
			size_t maxId = 0;
			for (const auto& building : existingBuildings) {
				if (building && building->getId() > maxId) {
					maxId = building->getId();
				}
			}
			newBuildingId = maxId + 1;
		}

		auto newBuilding = std::make_shared<ProductivityBuilding>(newBuildingId, playerId, tileId);
		tile->setBuildingId(playerId);
		this->gameState.addProductivityBuilding(newBuilding);
		player->addProductivityBuilding(newBuildingId);
		this->chargeResourceCost(*player, buildingCost);
		df::types::TilePotency current = tile->getPotency();

		df::types::TilePotency next = df::types::getNextPotency(current);
		if (current != next) {
			tile->setPotency(next);
			//fmt::println("Productivity increased! Tile {} is now {}", tile->getId(), df::types::potencyToString(next));
		}

		return true;
	}

	bool GameController::canUpgradeSettlement(size_t playerId, size_t settlementId, types::SettlementType targetType) const {
		const Settlement* settlement = this->findSettlementById(settlementId);
		if (!settlement || settlement->getPlayerId() != playerId) {
			return false;
		}

		const types::SettlementType currentType = settlement->getSettlementType();
		if (targetType == types::SettlementType::STONE && currentType != types::SettlementType::WOOD) {
			return false;
		}
		if (targetType == types::SettlementType::CASTLE && currentType != types::SettlementType::STONE) {
			return false;
		}

		return true;
	}

	bool GameController::upgradeSettlement(size_t playerId, size_t settlementId, types::SettlementType targetType, const std::vector<int>& buildingCost) {
		if (!canUpgradeSettlement(playerId, settlementId, targetType)) {
			return false;
		}

		Player* player = this->getPlayerbyId(playerId);
		if (!player) {
			return false;
		}
		if (!this->hasEnoughResources(*player, buildingCost)) {
			return false;
		}

		const auto settlements = this->gameState.getSettlements();
		for (const auto& settlement : settlements) {
			if (settlement && settlement->getId() == settlementId) {
				settlement->setSettlementType(targetType);
				break;
			}
		}

		this->gameState.syncSettlementType(settlementId, targetType);

		this->chargeResourceCost(*player, buildingCost);
		if( targetType == types::SettlementType::STONE){
			player->addHeroPoints(1);
		} else if ( targetType == types::SettlementType::CASTLE ){
			player->addHeroPoints(4);
		}

    	fmt::println("[GameController] Upgrade success. Player {} Hero Points: {}", playerId, player->getHeroPoints());
		return true;
	}

	int GameController::getCountCastles(size_t playerId) {
		Player* player = this->getPlayerbyId(playerId);
		if (!player) return 0;

		int castleCount = 0;
		const auto& settlementIds = player->getSettlementIds();

		for (size_t id : settlementIds) {
			for (const auto& settlement : gameState.getSettlements()) {
				if (settlement && settlement->getId() == id) {
					if (settlement->getSettlementType() == types::SettlementType::CASTLE) {
						castleCount++;
					}
					break;
				}
			}
		}
		return castleCount;
	}


	// TODO: move this functionality to settlement class
	std::vector<size_t> GameController::getSettlementTiles(const Settlement& settlement) const {
		std::vector<size_t> tileIds;
		const Graph& map = this->gameState.getMap();
		try {
			size_t vertexId = settlement.getVertexId();
			auto vertex = map.findVertexById(vertexId);
			if (!vertex) {
				return tileIds;
			}
			auto vertexTiles = map.getVertexTiles(vertex);

			if (!vertexTiles) {
				return tileIds;
			}

			for (const auto& tile : *vertexTiles) {
				if (!tile) {
					continue;
				}
				tileIds.push_back(tile->getId());
			}
		} catch (const std::exception&) {
		} // ignore invalid vert

		return tileIds;
	}


	// TODO: discuss where to put this...
	// Put this into vertex class? -> or better in settlement class as "hasNeighbourSettlement()"?!
	bool GameController::doesVertexHaveNeighborSettlements(size_t vertexId) const {
		const Graph& map = this->gameState.getMap();

		// TODO: FIX THIS: when settlement placed on "0", cannot build on "3"
		try {
			// Find vertex by ID (not index)
			VertexHandle vertex = map.findVertexById(vertexId);
			if (!vertex) {
				fmt::println("[GameController] doesVertexHaveNeighborSettlements: vertex {} not found", vertexId);
				return true; // block placement
			}

			// Check tiles that include this vertex + inspect the two adjacent vertices in each tile
			if (const auto tilesOpt = map.getVertexTiles(vertex)) {
				for (const auto& tile : *tilesOpt) {
					if (!tile || tile->getId() == SIZE_MAX)
						continue;

					const auto tileVerticesOpt = map.getTileVertices(tile);
					if (!tileVerticesOpt)
						continue;

					const auto& tileVertices = *tileVerticesOpt;
					for (size_t i = 0; i < tileVertices.size(); ++i) {
						if (tileVertices[i] != vertex)
							continue;

						const std::array<size_t, 2> neighboursIdx = {(i + 5) % 6, (i + 1) % 6};
						for (size_t idx : neighboursIdx) {
							const VertexHandle neighbour = tileVertices[idx];
							if (!neighbour || neighbour->getId() == SIZE_MAX || neighbour->getId() == vertexId)
								continue;
							if (neighbour->hasSettlement()) {
								fmt::println("[GameController] doesVertexHaveNeighborSettlements: neighbour settlement at vertex {} (tile {})",
											 neighbour->getId(), tile->getId());
								return true;
							}
						}
					}
				}
			}

			// TODO: There HAS to be a better solution...
			// Checking based on actual positoin
			const glm::vec2 targetPos = WorldNodeMapper::getWorldPositionForVertex(vertexId, map);
			const float neighbourThreshold = 1.05f; // float error tolerance
			for (const auto& vPtr : map.getVertices()) {
				if (!vPtr || !vPtr->hasSettlement())
					continue;

				const size_t otherId = vPtr->getId();
				if (otherId == vertexId)
					continue;

				const glm::vec2 otherPos = WorldNodeMapper::getWorldPositionForVertex(otherId, map);
				const float dist = glm::distance(targetPos, otherPos);
				if (dist <= neighbourThreshold) {
					fmt::println("[GameController] doesVertexHaveNeighborSettlements: geometry neighbour with settlement at vertex {} (dist {:.3f})",
								 otherId, dist);
					return true;
				}
			}
		} catch (const std::exception& e) {
			fmt::println("[GameController] doesVertexHaveNeighborSettlements: exception {} for vertex {}", e.what(), vertexId);
			return true; // block placement on error
		}

		return false;
	}


	// check if edge is connected with roads to a settlement from the player:
	// either the edge is directly connected to a settlement form the player
	// or the edge is connected to a road -> a road is always connected to a settlement
	bool GameController::doesEdgeConnectToPlayer(size_t playerId, size_t edgeId) const {
		const Graph& map = this->gameState.getMap();

		try {
			// Find edge by ID (not index)
			EdgeHandle edge = nullptr;
			for (size_t i = 0; i < map.getEdgeCount(); ++i) {
				if (map.getEdge(i)->getId() == edgeId) {
					edge = map.getEdge(i);
					break;
				}
			}
			if (!edge) {
				return false;
			}

			const auto verticesOpt = map.getEdgeVertices(edge);
			if (!verticesOpt)
				return false; // std::nullopt

			for (const auto& vertex : *verticesOpt) {
				if (vertex->hasSettlement()) {

					const auto settlementId = vertex->getSettlementId();
					if (settlementId.has_value()) {

						const Settlement* settlement = this->findSettlementById(settlementId.value());
						if (settlement && settlement->getPlayerId() == playerId) {
							return true;
						}
					}
				}

				const auto edgesOpt = map.getVertexEdges(vertex);
				if (!edgesOpt)
					continue;

				for (const auto& neighbourEdge : *edgesOpt) {
					if (neighbourEdge->getId() == SIZE_MAX || neighbourEdge->getId() == edgeId || !neighbourEdge->hasRoad()) {
						continue;
					}
					const auto roadId = neighbourEdge->getRoadId();
					if (!roadId.has_value()) {
						continue;
					}

					const Road* road = this->findRoadById(roadId.value());
					if (road && road->getPlayerId() == playerId) {
						return true;
					}
				}
			}
		} catch (const std::exception&) {
			return false;
		}

		return false;
	}


	bool GameController::canAfford(size_t playerId, const std::vector<int>& cost) const {
		const Player* player = this->getPlayerById(playerId);
		if (!player) {
			return false;
		}
		if (cost.empty()) {
			return true;
		}

		for (size_t i = 0; i < cost.size() && i < static_cast<size_t>(types::TileType::COUNT); ++i) {
			if (cost[i] > 0 && player->getResources(static_cast<types::TileType>(i)) < cost[i]) {
				return false;
			}
		}
		return true;
	}


	// TODO: changing buildingCost to a map (as is planned), would make this function rather obsolete
	bool GameController::hasEnoughResources(Player& player, const std::vector<int>& buildingCost) {
		if (buildingCost.empty()) {
			return true;
		} // building is free

		std::map<types::TileType, int> requirements;
		for (size_t i = 0; i < buildingCost.size() && i < static_cast<size_t>(types::TileType::COUNT); ++i) {
			if (buildingCost[i] > 0) {
				requirements[static_cast<types::TileType>(i)] = buildingCost[i];
			}
		}

		return (requirements.empty() || player.hasResources(requirements));
	}


	// TODO: when buildingCost is a map, this function would not be necessary anymore
	void GameController::chargeResourceCost(Player& player, const std::vector<int>& buildingCost) {
		if (buildingCost.empty()) {
			return;
		} // building is free -> nothing charged

		for (size_t i = 0; i < buildingCost.size() && i < static_cast<size_t>(types::TileType::COUNT); ++i) {
			if (buildingCost[i] > 0) {
				player.removeResources(static_cast<types::TileType>(i), buildingCost[i]);
			}
		}
	}


	// util functions
	const Road* GameController::findRoadById(size_t roadId) const {
		const auto& roads = this->gameState.getRoads();

		for (const auto& road : roads) {
			if (road && road->getId() == roadId) {
				return road.get();
			}
		}

		return nullptr;
	}


	// util functions
	const Settlement* GameController::findSettlementById(size_t settlementId) const {
		const auto& settlements = this->gameState.getSettlements();

		for (const auto& settlement : settlements) {
			if (settlement && settlement->getId() == settlementId) {
				return settlement.get();
			}
		}

		return nullptr;
	}

	const ProductivityBuilding* GameController::findProductivityBuildingById(size_t buildingId) const {
		const auto& buildings = this->gameState.getProductivityBuildings();

		for (const auto& building : buildings) {
			if (building && building->getId() == buildingId) {
				return building.get();
			}
		}

		return nullptr;
	}

	const ProductivityBuilding* GameController::findProductivityBuildingByTileId(size_t tileId) const {
		const auto& buildings = this->gameState.getProductivityBuildings();

		for (const auto& building : buildings) {
			if (building && building->getTileId() == tileId) {
				return building.get();
			}
		}

		return nullptr;
	}

	void GameController::claimQuestReward(int questId) {
		Player* player = this->getCurrentPlayer();
		QuestsSystem* quests = this->getQuestsSystem();

		if (!player || !quests)
			return;

		const Quest* q = quests->getQuestById(questId);

		if (q && q->state == QuestState::Completed) {
			player->addResources(q->reward_resource, q->reward_amount);
			quests->claimQuest(questId, player, &gameState);
		}
	}

	bool GameController::claimQuestRewardFor(size_t playerId, int questId) {
		Player* player = this->getPlayerbyId(playerId);
		QuestsSystem* quests = this->getQuestsSystem();
		if (!player || !quests || !quests->prepareClaim(questId)) {
			return false;
		}

		const Quest* quest = quests->getQuestById(questId);
		if (!quest || quest->state != QuestState::Completed) {
			return false;
		}

		player->addResources(quest->reward_resource, quest->reward_amount);
		quests->claimQuest(questId, player, &gameState);
		return true;
	}

} // namespace df
