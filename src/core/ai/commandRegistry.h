#pragma once
#include <string>
#include <unordered_map>

#include "behaviorTree.h"

namespace df {
	class CommandRegistry {
	public:
		// See https://stackoverflow.com/a/1008289 for reference
		static CommandRegistry& getInstance() {
			static auto instance = CommandRegistry();
			return instance;
		}
		CommandRegistry(CommandRegistry const&) = delete;
		void operator=(CommandRegistry const&) = delete;

		void registerCommand(const std::string& name, const std::function<BTState(Agent)>& function);
		bool hasCommand(const std::string& name) const;
		std::function<BTState(Agent)> getCommand(const std::string& name) const;
	private:
		CommandRegistry() = default;
		std::unordered_map<std::string, std::function<BTState(Agent)>> commands{};
	};
} // df
