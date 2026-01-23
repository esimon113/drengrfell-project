#include "aiSystem.h"

namespace df {
	AiSystem::AiSystem() {
		loadBehaviorTrees();
	}

	void AiSystem::loadCommands() {
		this->commands.registerCommand(
			"print",
			[](Agent, const CommandRegistry::Args &a) {
				std::cout << std::get<std::string>(a.at("text")) << std::endl;
				return BTState::Success;
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
