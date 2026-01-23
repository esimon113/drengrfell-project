#include "aiSystem.h"

namespace df {
	AiSystem::AiSystem() {
		this->commands.registerCommand(
			"print_hello",
			[](Agent) {
				std::cout << "Hello, World!" << std::endl;
				return BTState::Success;
			}
		);
		auto result = BTNode::deserialize(this->commands, assets::JsonFile::AI_BEHAVIOR_TREE_HERO);
		if (result.isErr()) {
			std::cerr << result.unwrapErr() << std::endl;
		} else {
			btRoot = result.unwrap<>();
		}
	}

	void AiSystem::onKeyCallback(GLFWwindow* /*window*/, const int key, int /*scancode*/, const int action, int /*mods*/) noexcept {
		if (action == GLFW_PRESS) {
			if (key == GLFW_KEY_P) {
				fmt::println("{}", btRoot->serialize().dump());
			}
		}
	}
}
