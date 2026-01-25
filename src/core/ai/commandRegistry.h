#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <variant>
#include <utility>

#include "btContext.h"

namespace df {
	class CommandRegistry {
	public:
		CommandRegistry() = default;
		~CommandRegistry() = default;

		void registerCommand(const std::string& name, const BTF::Command& command);
		// Like disconnect in events/signal.h
		// Needed when capturing a this-pointer
		void unregisterCommand(const std::string& name);
		bool hasCommand(const std::string& name) const;
		BTF::Command getCommand(const std::string& name) const;
	private:
		std::unordered_map<std::string, BTF::Command> commands{};
	};
} // df
