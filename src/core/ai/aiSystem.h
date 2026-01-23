#pragma once
#include <window.h>
#include "commandRegistry.h"

namespace df {
	class AiSystem {
	public:
		AiSystem(/*Registry* registry,*/ /*std::shared_ptr<GameState> gameState,*/ /*GameController* gameController*/);
		~AiSystem() = default;

		/*void step(float dt) noexcept;*/

		void loadCommands();
		Result<void, ResultError> loadBehaviorTrees();

		void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

		CommandRegistry& getCommandRegistry() { return commands; };

	private:
		CommandRegistry commands{};
		bool commandsLoaded{false};

		std::shared_ptr<BTNode> btRoot{};
	};
}
