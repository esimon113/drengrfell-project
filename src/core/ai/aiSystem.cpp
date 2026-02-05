#include "aiSystem.h"

#include <iostream>

#include "behaviorTree.h"
#include "resultError.h"
#include "fmt/color.h"
#include "fmt/os.h"

namespace df {
	AiSystem::AiSystem(Registry* registry) : registry(registry) {}

	AiSystem::~AiSystem() {
		this->commands.unregisterCommand("run");
		this->commands.unregisterCommand("getUniformInt");
	}

	void AiSystem::loadCommands() {
		this->commands.registerCommand(
			"run",
			[this](BTContext& context, const BTF::Args &args) {
				auto filename = BTF::getArg<BTString>(args, "file", "");
				if (filename.empty()) {
					std::cerr << "[AI Error]: Missing 'filename'" << std::endl;
					return BTState::Invalid;
				}
				auto result = this->loadBehaviorTree(filename);
				if (result.isOk()) {
					std::cout << "[AI]: Run '" << filename << "'" << std::endl;
					return loadedRoots[filename]->process(context);
				} else {
					std::cerr << "[AI Error]: Could not load tree " << filename << std::endl;
					return BTState::Invalid;
				}
			}
		);
		this->commands.registerCommand(
			"print",
			[](BTContext& /*context*/, const BTF::Args &a) {
				std::cout << "[AI]: " << BTF::getArg<std::string>(a, "text", "Missing 'text'") << std::endl;
				return BTState::Success;
			}
		);
		this->commands.registerCommand(
			"error",
			[](BTContext& /*context*/, const BTF::Args &args) {
				std::cerr << "[AI Error]: " << BTF::getArg<std::string>(args, "text", "Missing 'text'") << std::endl;
				return BTState::Invalid;
			}
		);
		this->commands.registerCommand(
			"store",
			[](BTContext& context, const BTF::Args &args) {
				auto varname = BTF::getArg<BTString>(args, "var", "ans");
				BTValueType varvalue;
				if (BTF::hasArg(args, "val")) {
					if (BTF::isArg<BTString>(args, "val")) {
						varvalue = BTF::getArg<BTString>(args, "val", "");
						fmt::println("[AI]: Stored {} into {}", std::get<std::string>(varvalue), varname);
					} else if (BTF::isArg<BTNumber>(args, "val")) {
						varvalue = BTF::getArg<BTNumber>(args, "val", 0.0);
						fmt::println("[AI]: Stored {} into {}", std::get<double>(varvalue), varname);
					} else if (BTF::isArg<BTBoolean>(args, "val")) {
						varvalue = BTF::getArg<BTBoolean>(args, "val", false);
						fmt::println("[AI]: Stored {} into {}", std::get<bool>(varvalue), varname);
					} else if (BTF::isArg<BTArray>(args, "val")) {
						varvalue = BTF::getArg<BTArray>(args, "val", BTArray{});
						fmt::println("[AI]: Stored {} into {}", to_string(varvalue.serialize()), varname);
					} else if (BTF::isArg<BTObject>(args, "val")) {
						varvalue = BTF::getArg<BTObject>(args, "val", BTObject{});
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
			[this](BTContext& context, const BTF::Args &args) {
				const int start = static_cast<int>(BTF::getArg<BTNumber>(args, "start", 0));
				const int end = static_cast<int>(BTF::getArg<BTNumber>(args, "end", 1));

				std::uniform_int_distribution<int> distrib(start, end);
				const auto res = distrib(this->mersenne_twister_engine);
				return BTF::store<BTNumber>(context, args, res);
			}
		);
		this->commands.registerCommand(
			"get",
			[](BTContext& context, const BTF::Args &args) {
				if (args.contains("array")) {
					const auto arr = BTF::getArg<BTArray>(args, "array", BTArray{});
					const auto idx = static_cast<int>(BTF::getArg<BTNumber>(args, "index", 0));
					if (arr.empty()) {
						return BTState::Failed;
					} else {
						return BTF::store<BTValueType>(context, args, *arr[idx % arr.size()]);
					}
				} else if (args.contains("object")) {
					const auto obj = BTF::getArg<BTObject>(args, "object", BTObject{});
					const auto key = BTF::getArg<BTString>(args, "key", BTString{});
					if (obj.contains(key)) {
						return BTF::store<BTValueType>(context, args, *obj.at(key));
					} else {
						return BTState::Failed;
					}
				} else {
					std::cerr << "[AI Error]: Invalid get node args. Does not contain an indexable 'array' or 'object'." << std::endl;
					return BTState::Invalid;
				}
			}
		);
		this->commands.registerCommand(
			"size",
			[](BTContext& context, const BTF::Args &args) {
				if (args.contains("array")) {
					const auto arr = BTF::getArg<BTArray>(args, "array", BTArray{});
					BTF::store<BTNumber>(context, args, arr.size());
				} else if (args.contains("object")) {
					const auto obj = BTF::getArg<BTObject>(args, "object", BTObject{});
					BTF::store<BTNumber>(context, args, obj.size());
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
		return loadBehaviorTree("ai_bt_hero");
	}

	Result<void, ResultError> AiSystem::loadBehaviorTree(const std::string& filename, bool skipIfLoaded) {
		if (skipIfLoaded && this->loadedRoots.contains(filename)) {
			return Ok();
		} else {
			auto result = BTNode::deserialize(this->commands, filename);
			if (result.isErr()) {
				return Err(result.unwrapErr());
			} else {
				this->loadedRoots[filename] = result.unwrap();
				fmt::println("[Ai]: Loaded BT: {}", this->loadedRoots[filename]->serialize().dump());
				return Ok();
			}
		}

	}

	void AiSystem::onKeyCallback(GLFWwindow* /*window*/, const int key, int /*scancode*/, const int action, int /*mods*/) {
		if (action == GLFW_PRESS) {
			if (key == GLFW_KEY_L) {
				aiActive = !aiActive;
				fmt::println("[Ai]: AI active set to {}", aiActive);
			}
			if (key == GLFW_KEY_P) {
				if (registry) {
					loadBehaviorTrees();
					const Agent p = registry->animations.entities.front();
					fmt::println("Agent P is {}", static_cast<int>(p));
					BTContext context {p};
					fmt::println("{}", to_string(this->loadedRoots["ai_bt_hero"]->process(context)));
				}
			}
		}
	}

	void AiSystem::processHero(Agent a) {
		if (registry) {
			if (!this->commandsLoaded) {
				loadCommands();
			}
			loadBehaviorTree("ai_bt_hero");
			BTContext context {a};
			fmt::println("{}", to_string(this->loadedRoots["ai_bt_hero"]->process(context)));
		}
	}
}
