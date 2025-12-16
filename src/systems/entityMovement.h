#pragma once
#include <common.h>
#include <registry.h>
#include <core/gamestate.h>
#include <glm/vec2.hpp>

namespace df {
	class EntityMovementSystem {
	public:
		static EntityMovementSystem init(Registry* registry, GameState& gameState) noexcept;

		void moveEntityTo(Entity entity, const glm::vec2& targetPosition, float deltaTime) noexcept;

	private:
		Registry* registry;
		GameState* gameState;

		glm::vec2 getTileWorldPosition(size_t tileIndex) const noexcept;
		size_t getTileIndexFromPosition(const glm::vec2& worldPosition) const noexcept;
	};
}