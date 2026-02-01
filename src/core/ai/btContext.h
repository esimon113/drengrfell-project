#pragma once
#include <string>
#include <any>
#include "tiny_ecs.hpp"


namespace df {
	typedef Entity Agent;

	enum class BTState {
		Invalid,
		Running,
		Success,
		Failed,
	};

	inline std::string_view to_string(const BTState state) {
		switch(state){
			case BTState::Invalid:
				return "INVALID";
			case BTState::Running:
				return "RUNNING";
			case BTState::Success:
				return "SUCCESS";
			case BTState::Failed:
				return "FAILED";
		}
		return "UNKNOWN";
	}

	using BTValueType = std::variant<std::string, double, bool>;

	struct BTStorage {
		std::unordered_map<std::string, BTValueType> data{};
	};

	struct BTContext {
		Agent entity{};
		BTStorage storage{};
	};

	namespace BTF {
		using Args = std::unordered_map<std::string, BTValueType>;
		using Command = std::function<BTState(BTContext&, Args)>;

		// Function for safe keyword argument lookup
		template<typename T>
		T getArg(const Args& args, const std::string& key, T defaultValue) {
			if (const auto iterator = args.find(key); iterator != args.end()) {
				if (std::holds_alternative<T>(iterator->second)) {
					return std::get<T>(iterator->second);
				}
			}
			return defaultValue;
		}

		template<typename T>
		bool isArg(const Args& args, const std::string& key) {
			if (const auto iterator = args.find(key); iterator != args.end()) {
				return std::holds_alternative<T>(iterator->second);
			}
			return false;
		}

		inline bool hasArg(const Args& args, const std::string& key) {
			if (const auto iterator = args.find(key); iterator != args.end()) {
				return true;
			}
			return false;
		}

		inline BTValueType evaluateString(const BTContext& context, const std::string& input) {
			if (input.starts_with("$")) {
				auto actual = input.substr(1);
				if (context.storage.data.contains(actual)) {
					return context.storage.data.at(actual);
				} else {
					return {};
				}
			} else {
				return input;
			}
		}
	}
}
