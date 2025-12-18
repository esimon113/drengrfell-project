#include "entityMovement.h"
#include "application.h"

namespace df {
	EntityMovementSystem EntityMovementSystem::init(Registry* registry, GameState& gameState) noexcept {
		EntityMovementSystem self;
		self.registry = registry;
		self.gameState = &gameState;

		return self;
	}

	void EntityMovementSystem::moveEntityTo(Entity entity, const glm::vec2& targetPos, float deltaTime) noexcept {
		if (!registry) return;

		auto& animComp = registry->animations.get(entity);
		glm::vec2& currentPos = registry->positions.get(entity);

		glm::vec2 direction = targetPos - currentPos;
		float distance = glm::length(direction);
		// if we are already there
		if (distance == 0.0f) {
			moving = false;
			movementState = false;
			return;
		}


		direction = glm::normalize(direction);
		float speed = 0.7f; // speed in tiles per second
		glm::vec2 movement = direction * speed * deltaTime;

		moving = true;
			
		if (glm::length(movement) >= distance) {
			currentPos = targetPos;
			
			animComp.currentType = Hero::AnimationType::Idle;
			animComp.anim.setCurrentFrameIndex(0);
			moving = false;
			movementState = false;
		}
		else {
			if (animComp.currentType == Hero::AnimationType::Idle) {
			animComp.currentType = Hero::AnimationType::Run;
			animComp.anim.setCurrentFrameIndex(0);
			}
			currentPos += movement;
		}
	}

	void EntityMovementSystem::toggleMovementState() noexcept {
		movementState = !movementState;
	}


	glm::vec2 EntityMovementSystem::getTileWorldPosition(size_t tileIndex) const noexcept {
		if (!gameState) return glm::vec2(0.0f);

		const Graph& map = gameState->getMap();
		unsigned mapWidth = map.getMapWidth();

		if (mapWidth != 0 && tileIndex < map.getTileCount()) {
			unsigned row = tileIndex / mapWidth;
			unsigned col = tileIndex % mapWidth;

			float x = 2.0f * (static_cast<float>(col) + 0.5f * (row & 1));
			float y = 1.5f * static_cast<float>(row);

			return glm::vec2(x, y);
		}
		else {
			return glm::vec2(0.0f);
		}
	}

	size_t EntityMovementSystem::getTileIndexFromPosition(const glm::vec2& worldPosition) const noexcept {
		if (!gameState) return 0;

		const Graph& map = gameState->getMap();
		unsigned mapWidth = map.getMapWidth();

		float rowF = worldPosition.y / 1.5f;
		unsigned row = static_cast<unsigned>(rowF);

		float colF = worldPosition.x / 2.0f - 0.5f * (row & 1);
		unsigned col = static_cast<unsigned>(colF);

		if (row >= map.getTileCount() / mapWidth || col >= mapWidth) return 0;

		return row * mapWidth + col;
	}

}