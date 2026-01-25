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

		// Function for safe keyword argument lookup
		template<typename T>
		static T getArg(const BTF::Args& args, const std::string& key, T defaultValue) {
			if (auto iterator = args.find(key); iterator != args.end()) {
				if (std::holds_alternative<T>(iterator->second)) {
					return std::get<T>(iterator->second);
				}
			}
			return defaultValue;
		}
	private:
		std::unordered_map<std::string, BTF::Command> commands{};
	};
} // df
