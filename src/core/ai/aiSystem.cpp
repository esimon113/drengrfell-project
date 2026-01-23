#include "aiSystem.h"

#include "fmt/color.h"

namespace df {
	AiSystem::AiSystem() {
		loadBehaviorTrees();
	}

	void AiSystem::loadCommands() {
		this->commands.registerCommand(
			"print",
			[](Agent, const CommandRegistry::Args &a) {
				std::cout << CommandRegistry::getArg<std::string>(a, "text", "Missing 'text'") << std::endl;
				return BTState::Success;
			}
		);
		this->commands.registerCommand(
			"error",
			[](Agent, const CommandRegistry::Args &a) {
				std::cerr << CommandRegistry::getArg<std::string>(a, "text", "Missing 'text'") << std::endl;
				return BTState::Failed;
			}
		);
		this->commandsLoaded = true;
	}

	Result<void, ResultError> AiSystem::loadBehaviorTrees() {
		if (!this->commandsLoaded) {
			loadCommands();
		}
		auto result = BTNode::deserialize(this->commands, assets::JsonFile::AI_BEHAVIOR_TREE_HERO);
		if (result.isErr()) {
			return Err(result.unwrapErr());
		} else {
			this->btRoot = result.unwrap<>();
			return Ok();
		}
	}

	void AiSystem::onKeyCallback(GLFWwindow* /*window*/, const int key, int /*scancode*/, const int action, int /*mods*/) {
		if (action == GLFW_PRESS) {
			if (key == GLFW_KEY_L) {
				fmt::println("{}", this->btRoot->serialize().dump());
			}
			if (key == GLFW_KEY_P) {
				fmt::println("{}", to_string(this->btRoot->process(Agent())));
			}
		}
	}
}
