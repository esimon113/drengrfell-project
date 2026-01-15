#include "commandRegistry.h"

namespace df {
	void CommandRegistry::registerCommand(const std::string &name, const std::function<BTState(Agent)> &function) {
		this->commands[name] = function;
	}

	bool CommandRegistry::hasCommand(const std::string &name) const {
		return this->commands.contains(name);
	}

	std::function<BTState(Agent)> CommandRegistry::getCommand(const std::string &name) const {
		return this->commands.at(name);
	}
} // df
