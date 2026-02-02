#pragma once
#include "commandRegistry.h"
#include "registry.h"
#include "resultError.h"
#include "behaviorTree.h"

namespace df {
	class AiSystem {
	public:
		explicit AiSystem(Registry* registry/*,std::shared_ptr<GameState> gameState,*/ /*GameController* gameController*/);
		~AiSystem();

		/*void step(float dt) noexcept;*/

		void loadCommands();
		Result<void, ResultError> loadBehaviorTrees();
		Result<void, ResultError> loadBehaviorTree(const std::string& filename, bool skipIfLoaded = true);

		void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

		CommandRegistry& getCommandRegistry() { return commands; };

	private:
		Registry* registry;
		CommandRegistry commands{};
		bool commandsLoaded{false};

		std::unordered_map<std::string, std::shared_ptr<BTNode>> loadedRoots{};
		std::mt19937 mersenne_twister_engine{std::random_device()()};
	};
}
