#pragma once
#include <common.h>
#include <core/gamestate.h>
#include <glm/vec2.hpp>
#include <registry.h>


namespace df {
	class AiSystem;
	class EntityMovementSystem {
	  public:
		explicit EntityMovementSystem(Registry* registry, const std::shared_ptr<GameState> &gameState, const std::shared_ptr<AiSystem>& aiSystem);
		~EntityMovementSystem();

		void moveEntityTo(Entity entity, const glm::vec2& targetPosition, float deltaTime) noexcept;

		glm::vec2 computeControlPoint(const glm::vec2& start, const glm::vec2& end) const noexcept;
		glm::vec2 quadraticBezier(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, float t) const noexcept;
		glm::vec2 getTileWorldPosition(size_t tileIndex) const noexcept;
		size_t getTileIndexFromPosition(const glm::vec2& worldPosition) const noexcept;

		void toggleMovementState() noexcept;
		void toggleTargetSet() noexcept;

		bool getMovementState() noexcept { return movementState; };
		bool isEntityMoving() const noexcept { return moving; }
		bool isTargetSet() const noexcept { return targetSet; }
		std::vector<size_t> getCurrentPath() const noexcept { return currentPath; };

		glm::vec2 getTargetPosition() const noexcept { return targetPosition; }
		void setTarget(const size_t id, Entity entity, Player* player) noexcept;

		unsigned getTileIDFromWorldPosition(const glm::vec2& worldPos) const noexcept;
    	void updateTileAndDiscover(Entity entity, unsigned tileID) noexcept;

	  private:
		Registry* registry;
		std::shared_ptr<GameState> gameState;
		std::shared_ptr<AiSystem> aiSystem;

		float speed = 1.5f; // speed in tiles per second
		bool movementState = false;
		bool moving = false;
		bool targetSet = false;
		glm::vec2 targetPosition = glm::vec2(0.0f);
		size_t targetPositionTileID{};

		std::vector<size_t> currentPath;
		size_t currentPathIndex = 0;

		float pathT = 0.0f; // bezier progress
		glm::vec2 bezierP0; // startpoint
		glm::vec2 bezierP1; // controllpoint
		glm::vec2 bezierP2; // endpoint
	};
} // namespace df
