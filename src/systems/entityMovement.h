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

		glm::vec2 getTileWorldPosition(size_t tileIndex) const noexcept;
		size_t getTileIndexFromPosition(const glm::vec2& worldPosition) const noexcept;

		void toggleMovementState() noexcept;
		void toggleTargetSet() noexcept;

		bool getMovementState() noexcept { return movementState; };
		bool isEntityMoving() const noexcept { return moving; }
		bool isTargetSet() const noexcept { return targetSet; }

		glm::vec2 getTargetPosition() const noexcept { return targetPosition; }
		void setTarget(const size_t id, Entity entity) noexcept;

		unsigned getTileIDFromWorldPosition(const glm::vec2& worldPos) const noexcept;
    	void updateTileAndDiscover(Entity entity, unsigned tileID) noexcept;

	  private:
		Registry* registry;
		std::shared_ptr<GameState> gameState;
		std::shared_ptr<AiSystem> aiSystem;

		bool movementState = false;
		bool moving = false;
		bool targetSet = false;
		glm::vec2 targetPosition = glm::vec2(0.0f);
		size_t targetPositionTileID{};
		std::vector<size_t> currentPath;
		size_t currentPathIndex = 0;
	};
} // namespace df
