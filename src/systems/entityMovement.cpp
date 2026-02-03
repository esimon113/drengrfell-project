#include "entityMovement.h"
#include "application.h"

namespace df {
	EntityMovementSystem::EntityMovementSystem(
		Registry* registry,
		const std::shared_ptr<GameState>& gameState,
		const std::shared_ptr<AiSystem>& aiSystem
	) : registry(registry), gameState(gameState), aiSystem(aiSystem) {
		// The capturing of the this-pointer renders the former init-method invalid
		if (!aiSystem) {
			fmt::print("EntityMovementSystem::EntityMovementSystem: aiSystem is null");
		}
		aiSystem->getCommandRegistry().registerCommand(
		"setMoveTarget",
		[this](const BTContext& context, const BTF::Args& a) {
			int target = glm::iround(BTF::getArg<double>(a, "id", 0.0));
			this->setTarget(target, context.entity);
			movementState = true;
			targetSet = true;
			fmt::println("Set move target of entity: {} to {}", static_cast<int>(context.entity), target);
			return BTState::Success;
		});
		aiSystem->getCommandRegistry().registerCommand(
		"getMapSize",
		[this](BTContext& context, const BTF::Args& args) {
			return BTF::store<BTNumber>(context, args, this->gameState->getMap().getTileCount());
		});
		aiSystem->getCommandRegistry().registerCommand(
		"getExploredTiles",
		[this](BTContext& context, const BTF::Args& args) {
			// TODO: Get correct player id
			Player* p = this->gameState->getPlayer(0);
			if (p) {
				const auto vec = p->getExploredTileIds();
				auto arr = BTArray{};
				arr.reserve(vec.size());
				for (const auto& id : vec) {
					BTValueType num = static_cast<BTNumber>(id);
					arr.emplace_back(std::make_shared<BTValueType>(num));
				}
				return BTF::store<BTArray>(context, args, arr);
			} else {
				std::cerr << "Failed to get player" << std::endl;
				return BTState::Failed;
			}
		});
		aiSystem->getCommandRegistry().registerCommand(
		"getUnexploredTiles",
		[this](BTContext& context, const BTF::Args& args) {
			// TODO: Get correct player id
			Player* p = this->gameState->getPlayer(0);
			if (p) {
				auto tileCount = this->gameState->getMap().getTileCount();

				std::vector<bool> explored(tileCount, false);
				for (size_t id : p->getExploredTileIds()) {
					explored[id] = true;
				}

				auto arr = BTArray{};
				arr.reserve(tileCount);
				for (size_t id = 0; id < tileCount; id++) {
					if (!explored[id]) {
						BTValueType num = static_cast<BTNumber>(id);
						arr.emplace_back(std::make_shared<BTValueType>(num));
					}
				}
				return BTF::store<BTArray>(context, args, arr);
			} else {
				std::cerr << "Failed to get player" << std::endl;
				return BTState::Failed;
			}
		});
	}

	EntityMovementSystem::~EntityMovementSystem() {
		aiSystem->getCommandRegistry().unregisterCommand("setMoveTarget");
		aiSystem->getCommandRegistry().unregisterCommand("getMapSize");
		aiSystem->getCommandRegistry().unregisterCommand("questsgetExploredTiles");
	}

	unsigned EntityMovementSystem::getTileIDFromWorldPosition(const glm::vec2& worldPos) const noexcept{
		const Graph& map = this->gameState->getMap();
		const unsigned columns = map.getMapWidth();

		const glm::ivec2 rowCol = RenderCommon::worldToRowColCoordinates(worldPos);
		int col = rowCol.x;
		int row = rowCol.y;

		if(col<0 || row<0 || col>= static_cast<int>(columns)){
			return 0;
		}

		return row*columns +col;

	}


	void EntityMovementSystem::updateTileAndDiscover(Entity entity, unsigned tileID) noexcept{
		if (registry->tileID.has(entity)) {
			registry->tileID.get(entity) = targetPositionTileID;
		} else {
			registry->tileID.emplace(entity, targetPositionTileID);
		}

		if (gameState) {
			Player* playerPtr = gameState->getPlayer(0);
			if (playerPtr) {
				if (playerPtr->exploreTile(tileID)) { 
					gameState->getMap().setRenderUpdateRequested(true);
					fmt::println("New Tile {} discovered!", tileID);
					
					auto* quests = registry->getSystem<QuestsSystem>();
					if (quests) {
						quests->updateProgress(types::QuestGoalType::DISCOVER, 1);
					}
				}
			}
		}

		fmt::println("Hero destination: {},{} | Stored TileID: {}",
						getTileWorldPosition(tileID).x,
						getTileWorldPosition(tileID).y,
						registry->tileID.get(entity));
	}


	void EntityMovementSystem::moveEntityTo(Entity entity, const glm::vec2& targetPos, float deltaTime) noexcept {
		if (!registry) {
			fmt::println("EntityMovementSystem::moveEntityTo: registry is null");
			return;
		}

		auto& animComp = registry->animations.get(entity);
		glm::vec2& currentPos = registry->positions.get(entity);
		glm::vec2& scale = registry->scales.get(entity);

		glm::vec2 direction = targetPos - currentPos;
		float distance = glm::length(direction);

		unsigned previousTileID = registry->tileID.has(entity) ? registry->tileID.get(entity) : 0;

		// if we are already there
		if (distance == 0.0f) {
			moving = false;
			movementState = false;
			targetSet = false;
			scale.x = 1.0f;
			updateTileAndDiscover(entity, targetPositionTileID);
        	return;
		}


		direction = glm::normalize(direction);
		float speed = 1.5f; // speed in tiles per second
		glm::vec2 movement = direction * speed * deltaTime;

		moving = true;

		if (direction.x > 0.0f) {
			scale.x = 1.0f;
		} else if (direction.x < 0.0f) {
			scale.x = -1.0f;
		}

		if (glm::length(movement) >= distance) {
			currentPos = targetPos;

			animComp.currentType = Hero::AnimationType::Idle;
			animComp.anim.setCurrentFrameIndex(0);
			moving = false;
			movementState = false;
			targetSet = false;

			if (registry->tileID.has(entity)) {
				registry->tileID.get(entity) = targetPositionTileID;
			} else {
				registry->tileID.emplace(entity, targetPositionTileID);
			}

			if (gameState) {
				Player* playerPtr = gameState->getPlayer(0);
				if (playerPtr) {
					if (playerPtr->exploreTile(targetPositionTileID)) { 
						gameState->getMap().setRenderUpdateRequested(true);
						fmt::println("New Tile {} discovered!", targetPositionTileID);
						
						auto* quests = registry->getSystem<QuestsSystem>();
						if (quests) {
							quests->updateProgress(types::QuestGoalType::DISCOVER, 1);
						}
					}
				}
			}

			fmt::println("Hero destination: {},{} | Stored TileID: {}",
						 getTileWorldPosition(targetPositionTileID).x,
						 getTileWorldPosition(targetPositionTileID).y,
						 registry->tileID.get(entity));

		} else {
			if (animComp.currentType == Hero::AnimationType::Idle) {
				animComp.currentType = Hero::AnimationType::Run;
				animComp.anim.setCurrentFrameIndex(0);
			}
			currentPos += movement;

			unsigned currentTileID = getTileIDFromWorldPosition(currentPos);
			if (currentTileID != previousTileID && currentTileID != 0) {
				updateTileAndDiscover(entity, currentTileID);
			}
		}
	}

	void EntityMovementSystem::toggleMovementState() noexcept {
		movementState = !movementState;
	}

	void EntityMovementSystem::toggleTargetSet() noexcept {
		targetSet = !targetSet;
	}

	void EntityMovementSystem::setTarget(const size_t id, Entity entity) noexcept {
		glm::vec2& currentPos = registry->positions.get(entity);
		size_t& currentPosTileId = registry->tileID.get(entity);

		// Same tile has been selected twice -> deselect it by choosing the current hero position as the new target -> hero stands still
		if (targetPositionTileID == id) {
			targetPosition = currentPos;
			targetSet = true;
			targetPositionTileID = currentPosTileId;
			fmt::println("Target deselected (same tile clicked twice)");
			return;
		}

		targetPosition = getTileWorldPosition(id);
		targetSet = true;
		targetPositionTileID = id;
		fmt::println("New target tile selected: {}", id);
	}

	glm::vec2 EntityMovementSystem::getTileWorldPosition(size_t tileIndex) const noexcept {
		if (!gameState)
			return glm::vec2(0.0f);

		const Graph& map = gameState->getMap();
		unsigned mapWidth = map.getMapWidth();

		if (mapWidth != 0 && tileIndex < map.getTileCount()) {
			unsigned row = tileIndex / mapWidth;
			unsigned col = tileIndex % mapWidth;

			float x = 2.0f * (static_cast<float>(col) + 0.5f * (row & 1));
			float y = 1.5f * static_cast<float>(row);

			return glm::vec2(x, y);
		} else {
			return glm::vec2(0.0f);
		}
	}

	size_t EntityMovementSystem::getTileIndexFromPosition(const glm::vec2& worldPosition) const noexcept {
		if (!gameState)
			return 0;

		const Graph& map = gameState->getMap();
		unsigned mapWidth = map.getMapWidth();

		float rowF = worldPosition.y / 1.5f;
		unsigned row = static_cast<unsigned>(rowF);

		float colF = worldPosition.x / 2.0f - 0.5f * (row & 1);
		unsigned col = static_cast<unsigned>(colF);

		if (row >= map.getTileCount() / mapWidth || col >= mapWidth)
			return 0;

		return row * mapWidth + col;
	}

} // namespace df
