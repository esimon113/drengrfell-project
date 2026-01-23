#pragma once
#include <string>
#include <unordered_map>

#include "behaviorTree.h"

namespace df {
	class CommandRegistry {
	public:
		using JsonType = std::variant<std::string, double, bool>;
		using Args = std::unordered_map<std::string, JsonType>;
		using Command = std::function<BTState(Agent, Args)>;

		CommandRegistry() = default;
		~CommandRegistry() = default;

		void registerCommand(const std::string& name, const Command& command);
		bool hasCommand(const std::string& name) const;
		Command getCommand(const std::string& name) const;

		// Function for safe keyword argument lookup
		template<typename T>
		static T getArg(const Args& args, const std::string& key, T defaultValue) {
			if (auto iterator = args.find(key); iterator != args.end()) {
				if (std::holds_alternative<T>(iterator->second)) {
					return std::get<T>(iterator->second);
				}
			}
			return defaultValue;
		}
	private:
		std::unordered_map<std::string, Command> commands{};
	};
} // df
