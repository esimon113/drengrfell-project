#pragma once
#include "commandRegistry.h"
#include "registry.h"
#include "resultError.h"
#include "behaviorTree.h"
#include "events/eventBus.h"

namespace df {
	class AiSystem {
	public:
		explicit AiSystem(Registry* registry, std::shared_ptr<EventBus> bus/*,std::shared_ptr<GameState> gameState,*/ /*GameController* gameController*/);
		~AiSystem();

		/*void step(float dt) noexcept;*/

		void loadCommands();
		Result<void, ResultError> loadBehaviorTrees();
		Result<void, ResultError> loadBehaviorTree(const std::string& filename, bool skipIfLoaded = true);

		void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

		CommandRegistry& getCommandRegistry() { return commands; };

		void processHero(Agent a);
		void setAiActive(const bool active) {aiActive = active;};
		bool isAiActive() const {return aiActive;};

	private:
		bool aiActive = false;

		Registry* registry;
		std::shared_ptr<EventBus> eventBus;
		CommandRegistry commands{};
		bool commandsLoaded{false};

		std::unordered_map<std::string, std::shared_ptr<BTNode>> loadedRoots{};
		std::mt19937 mersenne_twister_engine{std::random_device()()};
	};
}
