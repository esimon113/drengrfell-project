#include "entityMovement.h"

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
		if (distance == 0.0f) return;

		direction = glm::normalize(direction);
		float speed = 0.7f; // speed in tiles per second
		glm::vec2 movement = direction * speed * deltaTime;
			
		if (glm::length(movement) >= distance) {
			currentPos = targetPos;
			
			animComp.currentType = Hero::AnimationType::Idle;
			animComp.anim.setCurrentFrameIndex(0);
		}
		else {
			if (animComp.currentType == Hero::AnimationType::Idle) {
			animComp.currentType = Hero::AnimationType::Run;
			animComp.anim.setCurrentFrameIndex(0);
			}
			currentPos += movement;
		}
	}

	glm::vec2 EntityMovementSystem::getTileWorldPosition(size_t tileIndex) const noexcept {
		if (!gameState) return glm::vec2(0.0f);

		const Graph& map = gameState->getMap();
		unsigned mapWidth = map.getMapWidth();

		if (mapWidth != 0 && tileIndex < map.getTileCount()) {
			unsigned row = tileIndex / mapWidth;
			unsigned col = tileIndex % mapWidth;
			return glm::vec2(static_cast<float>(col), static_cast<float>(row));
		}
		else {
			return glm::vec2(0.0f);
		}
	}

	size_t EntityMovementSystem::getTileIndexFromPosition(const glm::vec2& worldPosition) const noexcept {
		if (!gameState) return 0;

		const Graph& map = gameState->getMap();
		unsigned mapWidth = map.getMapWidth();

		unsigned col = static_cast<unsigned>(worldPosition.x);
		unsigned row = static_cast<unsigned>(worldPosition.y);

		return row * mapWidth + col;
	}

}