#pragma once
#include <common.h>
#include <core/gamestate.h>
#include <glm/vec2.hpp>
#include <registry.h>


namespace df {
	class EntityMovementSystem {
	  public:
		static EntityMovementSystem init(Registry* registry, GameState& gameState) noexcept;

		void moveEntityTo(Entity entity, const glm::vec2& targetPosition, float deltaTime) noexcept;

		glm::vec2 getTileWorldPosition(size_t tileIndex) const noexcept;
		size_t getTileIndexFromPosition(const glm::vec2& worldPosition) const noexcept;

		void toggleMovementState() noexcept;
		void toggleTargetSet() noexcept;
		bool getMovementState() noexcept { return movementState; };
		bool isEntityMoving() const noexcept { return moving; }
		bool isTargetSet() const noexcept { return targetSet; }
		glm::vec2 getTargetPosition() const noexcept { return targetPosition; }

		void setTargetPosition(const glm::vec2& target) noexcept;

	  private:
		Registry* registry;
		GameState* gameState;
		bool movementState = false;
		bool moving = false;
		bool targetSet = false;
		glm::vec2 targetPosition = glm::vec2(0.0f);
	};
} // namespace df
