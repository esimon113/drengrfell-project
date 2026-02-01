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
			[](BTContext& /*context*/, const BTF::Args &a) {
				std::cout << BTF::getArg<std::string>(a, "text", "Missing 'text'") << std::endl;
				return BTState::Success;
			}
		);
		this->commands.registerCommand(
			"error",
			[](BTContext& /*context*/, const BTF::Args &a) {
				std::cerr << BTF::getArg<std::string>(a, "text", "Missing 'text'") << std::endl;
				return BTState::Failed;
			}
		);
		this->commands.registerCommand(
			"store",
			[](BTContext& context, const BTF::Args &a) {
				auto varname = BTF::getArg<BTString>(a, "var", "ans");
				BTValueType varvalue;
				if (BTF::hasArg(a, "val")) {
					if (BTF::isArg<BTString>(a, "val")) {
						varvalue = BTF::getArg<BTString>(a, "val", "");
						fmt::println("[AI]: Stored {} into {}", std::get<std::string>(varvalue), varname);
					} else if (BTF::isArg<BTNumber>(a, "val")) {
						varvalue = BTF::getArg<BTNumber>(a, "val", 0.0);
						fmt::println("[AI]: Stored {} into {}", std::get<double>(varvalue), varname);
					} else if (BTF::isArg<BTBoolean>(a, "val")) {
						varvalue = BTF::getArg<BTBoolean>(a, "val", false);
						fmt::println("[AI]: Stored {} into {}", std::get<bool>(varvalue), varname);
					} else if (BTF::isArg<BTArray>(a, "val")) {
						varvalue = BTF::getArg<BTArray>(a, "val", BTArray{});
						fmt::println("[AI]: Stored {} into {}", to_string(varvalue.serialize()), varname);
					} else if (BTF::isArg<BTObject>(a, "val")) {
						varvalue = BTF::getArg<BTObject>(a, "val", BTObject{});
						fmt::println("[AI]: Stored {} into {}", to_string(varvalue.serialize()), varname);
					}
				} else if (context.storage.data.contains("ans")) {
					varvalue = context.storage.data["ans"];
				} else {
					std::cerr << "[AI Error]: Invalid store node args. 'ans' is not set." << std::endl;
					return BTState::Invalid;
				}
				context.storage.data[varname] = varvalue;
				return BTState::Success;
			}
		);
		this->commands.registerCommand(
			"getUniformInt",
			[this](BTContext& context, const BTF::Args &a) {
				const int start = static_cast<int>(BTF::getArg<BTNumber>(a, "start", 0));
				const int end = static_cast<int>(BTF::getArg<BTNumber>(a, "end", 1));

				std::uniform_int_distribution<int> distrib(start, end);
				auto res = distrib(this->mersenne_twister_engine);
				context.storage.data["ans"] = static_cast<BTNumber>(res);
				return BTState::Success;
			}
		);
		this->commands.registerCommand(
			"get",
			[](BTContext& context, const BTF::Args &a) {
				if (a.contains("array")) {
					const auto arr = BTF::getArg<BTArray>(a, "array", BTArray{});
					const auto idx = static_cast<int>(BTF::getArg<BTNumber>(a, "index", 0));
					context.storage.data["ans"] = *arr[idx];
				} else if (a.contains("object")) {
					const auto obj = BTF::getArg<BTObject>(a, "object", BTObject{});
					const auto key = BTF::getArg<BTString>(a, "key", BTString{});
					context.storage.data["ans"] = *obj.at(key);
				} else {
					std::cerr << "[AI Error]: Invalid get node args. Does not contain an indexable 'array' or 'object'." << std::endl;
					return BTState::Invalid;
				}
				return BTState::Success;
			}
		);
		this->commands.registerCommand(
			"size",
			[](BTContext& context, const BTF::Args &a) {
				if (a.contains("array")) {
					const auto arr = BTF::getArg<BTArray>(a, "array", BTArray{});
					context.storage.data["ans"] = static_cast<BTNumber>(arr.size());
				} else if (a.contains("object")) {
					const auto obj = BTF::getArg<BTObject>(a, "object", BTObject{});
					context.storage.data["ans"] = static_cast<BTNumber>(obj.size());
				} else {
					std::cerr << "[AI Error]: Invalid size node args. Does not contain an 'array' or 'object'." << std::endl;
					return BTState::Invalid;
				}
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
				loadBehaviorTrees();
				fmt::println("Loaded BT: {}", this->btRoot->serialize().dump());
			}
			if (key == GLFW_KEY_P) {
				if (registry) {
					const Agent p = registry->animations.entities.front();
					fmt::println("Agent P is {}", static_cast<int>(p));
					BTContext context {p};
					fmt::println("{}", to_string(this->btRoot->process(context)));
				}
			}
		}
	}
}
