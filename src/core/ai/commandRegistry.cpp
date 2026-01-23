#include "commandRegistry.h"

namespace df {
	void CommandRegistry::registerCommand(const std::string &name, const Command& command) {
		this->commands[name] = command;
	}

	bool CommandRegistry::hasCommand(const std::string &name) const {
		return this->commands.contains(name);
	}

	CommandRegistry::Command CommandRegistry::getCommand(const std::string &name) const {
		return this->commands.at(name);
	}
} // df
