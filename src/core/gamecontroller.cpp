#include "edge.h"
#include "fmt/base.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <optional>
#include <stdexcept>
#include <unordered_set>

#include "gamecontroller.h"
#include "hero.h"
#include "renderNotification.h"
#include "tile.h"
#include "types.h"
#include "../systems/renderTiles.h"
#include "utils/worldNodeMapper.h"
#include "vertex.h"
#include "../systems/renderSnow.h"





namespace df {

	Player* GameController::getCurrentPlayer() { return this->getPlayerbyId(this->gameState.getCurrentPlayerId()); }
	const Player* GameController::getCurrentPlayer() const { return this->getPlayerById(this->gameState.getCurrentPlayerId()); }


	Player* GameController::getPlayerbyId(size_t playerId) { return this->gameState.getPlayer(playerId); }
	const Player* GameController::getPlayerById(size_t playerId) const { return this->gameState.getPlayer(playerId); }


	void GameController::startTurn(Registry& registry) {
		Player* player = this->getCurrentPlayer();
		if (!player) {
			fmt::println("Current Player does not exist!");
			return;
		}

		this->giveResourcesTo(*player);
		this->resetHeroMovement(*player);

		// Check hazards
		// TODO: For multiplayer only update hazards for current player/hero
		if (this->gameState.getTurnCount() > 0) {
			// Entity hero = registry.animations.entities.front();
			showHazards(registry);
		}
	}


	void GameController::endTurn(Registry& registry) {
		const size_t playerCount = this->gameState.getPlayerCount();
		if (playerCount == 0) {
			return;
		} // should not happen

		updateHazards(registry);

		// TODO: maybe add some "setNextTurn()" etc. functions
		size_t nextPlayerId = (this->gameState.getCurrentPlayerId() + 1) % playerCount;
		this->gameState.setCurrentPlayerId(nextPlayerId);
		this->gameState.setTurnCount(this->gameState.getTurnCount() + 1);


		auto* snowSystem = registry.getSystem<df::RenderSnowSystem>();
		if (snowSystem) {
			snowSystem->increaseIntensity();
			
		}

		if (nextPlayerId == 0) {
			this->gameState.setRoundNumber(this->gameState.getRoundNumber() + 1);
		}

		if(this->gameState.getTurnCount() == 10){
			auto* tileSystem = registry.getSystem<RenderTilesSystem>();
			if (tileSystem) {
				tileSystem->updateTileAtlas();
				
			}
		}
	}

	// This function checks if the hero encounters a hazard at the destination (in world coordinates)
	void GameController::applyHazard(Entity hero, Registry& registry, glm::vec2 destination) {
		// Hero is already caught in a hazard
		if (registry.hazards.has(hero)) {
			fmt::println("Hazard can not be applied, as hero already has hazard");
			return;
		}

		glm::vec2 pos = destination;
		fmt::println("Hero Position: ({},{})", pos.x, pos.y);

		TileHandle tile = this->gameState.getMap().getTileFromWorldPosition(pos.x, pos.y);
		if (!tile) {
			fmt::println("No tile for hazard checking found");
			return;
		}

		const auto& profileOpt = tile->getHazardProfile();
		if (!profileOpt) {
			fmt::println("No profile for hazard checking found");
			return;
		}

		const auto& profile = *profileOpt;

		std::uniform_real_distribution<float> dist(0.0f, 1.0f);
		bool encounteredHazard = dist(rng) <= profile.probability;

		if (!encounteredHazard) {
			fmt::println("No hazard encountered");
			return;
		}

		const auto& def = HazardDB::getDefinition(profile.hazardType);

		registry.hazards.emplace(hero) = {profile.hazardType, def.defaultRoundDuration};
		fmt::println("[Hazard] You encountered a {}, which will stop your movement for {} turns", def.name, def.defaultRoundDuration);
	}

	// TODO: Only update hazards for active player in multiplayer
	void GameController::updateHazards(Registry& registry) {
		for (Entity e : registry.hazards.entities) {
			auto& hazard = registry.hazards.get(e);
			auto hazardDefinition = HazardDB::getDefinition(hazard.type);

			hazard.turnsLeft--;
		}
	}

	void GameController::showHazards(Registry& registry) {
		for (Entity e : registry.hazards.entities) {
			auto& hazard = registry.hazards.get(e);
			auto hazardDefinition = HazardDB::getDefinition(hazard.type);
			RenderNotificationSystem* notification = registry.getSystem<RenderNotificationSystem>();

			if (hazard.turnsLeft <= 0) {
				fmt::println("[Hazard] {} encounter ended", hazardDefinition.name);
				notification->showNotification("You overcame the hazard",
											   fmt::format(
												   "Your encounter with the {} ended",
												   hazardDefinition.name),
											   {"Continue"});
				registry.hazards.remove(e);
			} else if (hazard.turnsLeft == hazardDefinition.defaultRoundDuration) {
				fmt::println("[Hazard] {} encountered. It is active for {} turns", hazardDefinition.name, hazard.turnsLeft);
				notification->showNotification("You encountered a hazard",
											   fmt::format(
												   "A {} is preventing you from moving for {} turns\n"
												   "Would you like to overcome the encounter by paying {} {} or wait?",
												   hazardDefinition.name,
												   hazard.turnsLeft,
												   hazardDefinition.skipCost * hazard.turnsLeft,
												   hazardDefinition.skipRessourceStr),
											   {"Pay ressources",
												"Wait"});
			} else {
				fmt::println("[Hazard] {} encounter ongoing. It is still active for {} turns", hazardDefinition.name, hazard.turnsLeft);
				notification->showNotification("Ongoing hazard",
											   fmt::format(
												   "A {} is still preventing you from moving for {} turns\n"
												   "Would you like to overcome the encounter by paying {} {} or wait?",
												   hazardDefinition.name,
												   hazard.turnsLeft,
												   hazardDefinition.skipCost * hazard.turnsLeft,
												   hazardDefinition.skipRessourceStr),
											   {"Pay ressources",
												"Wait"});
			}
		}
	}

	void GameController::payForHazard(Registry& registry) {
		for (Entity e : registry.hazards.entities) {
			auto& hazard = registry.hazards.get(e);
			auto hazardDefinition = HazardDB::getDefinition(hazard.type);
			RenderNotificationSystem* notification = registry.getSystem<RenderNotificationSystem>();

			Player* player = this->getCurrentPlayer();

			if (player->getResources(hazardDefinition.skipRessource) < hazard.turnsLeft * hazardDefinition.skipCost) {
				notification->showNotification("Not enough ressources",
											   fmt::format(
												   "You have {} {}, but need {} to overcome the hazard",
												   player->getResources(hazardDefinition.skipRessource),
												   hazardDefinition.skipRessourceStr,
												   hazard.turnsLeft * hazardDefinition.skipCost),
											   {"Continue"});
				return;
			}
			player->removeResources(hazardDefinition.skipRessource, hazard.turnsLeft * hazardDefinition.skipCost);
			registry.hazards.remove(e);
		}
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
					player.addResources(tile->getType(), 1); // TODO: make amount configurable -> i.e. in settlers of catan a town gives 2 resources

					std::string type = types::tileTypeToString(tile->getType());
					std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) { return std::tolower(c); });
					this->m_questsSystem->updateProgress(type, 1);
				}
			}
		}
	}


	void GameController::resetHeroMovement(Player& player) {
		std::shared_ptr<Hero> hero = player.getHero();
		if (hero) {
			// TODO: something like this needs to be implemented in hero class:
			// reset the available movement points, hero also needs to keep track of used range per turn
			// hero->resetMovementPoints();
			// hero->startIdleAnimation();
		}
	}


	void GameController::exploreTile(Player& player, size_t tileId) {
		Graph& map = this->gameState.getMap();

		try {
			TileHandle tile = map.getTile(tileId);

			if (!player.isTileExplored(tileId)) {
				tile->addVisibleForPlayers(player.getId());
				player.exploreTile(tileId);
			}
		} catch (const std::exception&) {
		} // invalid tile -> ignore
	}


	bool GameController::moveHeroToTile(size_t playerId, size_t targetTileId) {
		Player* player = this->getPlayerbyId(playerId);
		if (!player) {
			return false;
		}

		std::shared_ptr<Hero> hero = player->getHero();
		if (!hero) {
			return false;
		}

		const int currentTileId = hero->getTileID(); // TODO: use size_t in hero
		size_t distance = 0;
		if (currentTileId >= 0) {
			const Graph& map = this->gameState.getMap();
			const TileHandle currentTile = map.getTile(currentTileId);
			const TileHandle targetTile = map.getTile(targetTileId);
			distance = map.getDistanceBetween(currentTile, targetTile);

			if (distance == SIZE_MAX) {
				return false;
			}
		}

		// TODO: hero class should implement moving the hero to a specified tile.
		// TODO: use size_t in hero -> id and distance cannot be negative -> make this information explicit by used datatype
		// if (!hero->moveToTile(targetTileId, distance)) { return false; }

		auto range = hero->getBaseRange();
		auto remainingRange = range - static_cast<int>(distance);
		// TODO: set remaining range for the hero for the current turn
		// otherwise we need to specify that the hero can be moved only once per turn
		// something like this:
		// hero->setRemainingRange(remainingRange);

		// TODO: this is only a temporary workaround:
		if (remainingRange < 0) {
			return false;
		}

		this->exploreTile(*player, targetTileId);

		return true; // success
	}


	bool GameController::canBuildSettlement(size_t playerId, size_t vertexId) const {
		(void)playerId; // unused for now - simplified building rules
		const Graph& map = this->gameState.getMap();
		try {
			fmt::println("[GameController] canBuildSettlement: checking vertex {}", vertexId);
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
			fmt::println("[GameController] canBuildSettlement: vertex {} is a valid placement", vertexId);
			return true;
		} catch (const std::exception&) {
			return false;
		}
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
		// Tutorial
		auto* step = this->gameState.getCurrentTutorialStep();

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

			// this->chargeResourceCost(*player, newSettlement->getBuildingCost());
			this->chargeResourceCost(*player, buildingCost);

			m_questsSystem->updateProgress("settlement", 1);


			fmt::println("[GameController] buildSettlement succeeded: settlement {} built at vertex {} for player {}", newSettlementId, vertexId, playerId);
			// Finish Tutorial if step is BUILD_SETTLEMENT
			if (step && step->id == TutorialStepId::BUILD_SETTLEMENT) {
				this->gameState.completeCurrentTutorialStep();
			}

			return true;

		} catch (const std::exception& e) {
			fmt::println("[GameController] buildSettlement failed: exception - {}", e.what());
			return false;
		}
	}


	// TODO: validate this in edge class
	bool GameController::canBuildRoad(size_t playerId, size_t edgeId) const {
		(void)playerId; // unused for now - simplified building rules
		const Graph& map = this->gameState.getMap();

		try {
			// Find edge by ID (not index)
			EdgeHandle edge = map.findEdgeById(edgeId);
			if (!edge) {
				return false;
			}

			// Only check if edge already has a road
			if (edge->hasRoad()) {
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
								fmt::println("[GameController] canBuildRoad: edge {} connects same vertices as edge {} which has a road",
											 edgeId, otherEdge->getId());
								return false; // Another edge at the same location already has a road
							}
						}
					}
					// If edgeVertexIds.size() != 2, we skip the duplicate check and allow building
				}
				// If verticesOpt is nullopt, we also allow building (edge case)
			} catch (const std::exception& e) {
				// If the duplicate check fails for any reason, we still allow building
				// This is a safeguard check and shouldn't block legitimate road building
				fmt::println("[GameController] canBuildRoad: duplicate check failed for edge {}: {}, allowing building", edgeId, e.what());
			}

			return true;
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
		// Tutorial
		auto* step = this->gameState.getCurrentTutorialStep();
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

			m_questsSystem->updateProgress("road", 1);

			fmt::println("[GameController] buildRoad succeeded: road {} built at edge {} for player {}", roadId, edgeId, playerId);
			// Finish Tutorial if step is BUILD_ROAD
			if (step && step->id == TutorialStepId::BUILD_ROAD) {
				this->gameState.completeCurrentTutorialStep();
			}

			return true;

		} catch (const std::exception& e) {
			fmt::println("[GameController] buildRoad failed: exception - {}", e.what());
			return false;
		}
	}


	// TODO: move this functionality to settlement class
	std::vector<size_t> GameController::getSettlementTiles(const Settlement& settlement) const {
		std::vector<size_t> tileIds;
		const Graph& map = this->gameState.getMap();
		try {
			size_t vertexId = settlement.getVertexId();
			auto vertex = map.findVertexById(vertexId);
			auto vertexTiles = map.getVertexTiles(vertex);

			if (!vertexTiles) {
				return tileIds;
			}

			for (const auto& tile : *vertexTiles) {
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

	void GameController::claimQuestReward(int questId) {
		Player* player = this->getCurrentPlayer();
		QuestsSystem* quests = this->getQuestsSystem();

		if (!player || !quests)
			return;

		const Quest* q = quests->getQuestById(questId);

		if (q && q->state == QuestState::Completed) {

			player->addResources(q->reward_resource, q->reward_amount);

			quests->claimQuest(questId, player);

			fmt::println("Reward given to the  player: {} units of type {}", q->reward_amount, (int)q->reward_resource);
		}
	}

} // namespace df
