#pragma once
#include <string>
#include <unordered_map>

#include "behaviorTree.h"

namespace df {
	class CommandRegistry {
	public:
		CommandRegistry() = default;
		~CommandRegistry() = default;

		void registerCommand(const std::string& name, const std::function<BTState(Agent)>& function);
		bool hasCommand(const std::string& name) const;
		std::function<BTState(Agent)> getCommand(const std::string& name) const;
	private:
		std::unordered_map<std::string, std::function<BTState(Agent)>> commands{};
	};
} // df
