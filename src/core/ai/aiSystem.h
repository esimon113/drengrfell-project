#pragma once
#include <window.h>
#include "commandRegistry.h"

namespace df {
	class AiSystem {
	public:
		AiSystem(/*Registry* registry,*/ /*std::shared_ptr<GameState> gameState,*/ /*GameController* gameController*/);
		~AiSystem() = default;

		/*void step(float dt) noexcept;*/

		void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;

	private:
		CommandRegistry commands{};
		std::shared_ptr<BTNode> btRoot{};
	};
}
