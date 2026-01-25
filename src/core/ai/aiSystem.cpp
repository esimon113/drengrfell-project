#include "aiSystem.h"

#include <iostream>

#include "behaviorTree.h"
#include "resultError.h"
#include "fmt/color.h"

namespace df {
	AiSystem::AiSystem(Registry* registry) : registry(registry) {}

	void AiSystem::loadCommands() {
		this->commands.registerCommand(
			"print",
			[](BTContext /*context*/, const BTF::Args &a) {
				std::cout << CommandRegistry::getArg<std::string>(a, "text", "Missing 'text'") << std::endl;
				return BTState::Success;
			}
		);
		this->commands.registerCommand(
			"error",
			[](BTContext /*context*/, const BTF::Args &a) {
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
				loadBehaviorTrees();
				fmt::println("Loaded BT: {}", this->btRoot->serialize().dump());
			}
			if (key == GLFW_KEY_P) {
				if (registry) {
					Agent p = registry->animations.entities.front();
					fmt::println("Agent P is {}", int(p));
					fmt::println("{}", to_string(this->btRoot->process(BTContext(p))));
				}
			}
		}
	}
}
