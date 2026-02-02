#pragma once
#include <string>
#include <memory>
#include <any>
#include <utility>
#include "tiny_ecs.hpp"
#include "jsonUtils.h"

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

	struct BTValueType;
	using BTString = std::string;
	using BTNumber = double;
	using BTBoolean = bool;
	using BTArray = std::vector<std::shared_ptr<BTValueType>>;
	using BTObject = std::unordered_map<std::string, std::shared_ptr<BTValueType>>;
	struct BTValueType: std::variant<BTString, BTNumber, BTBoolean, BTArray, BTObject> {
		using variant::variant;

		inline nlohmann::json serialize() {
			if (std::holds_alternative<BTString>(*this)) {
				return std::get<BTString>(*this);
			} else if (std::holds_alternative<BTNumber>(*this)) {
				return std::get<BTNumber>(*this);
			} else if (std::holds_alternative<BTBoolean>(*this)) {
				return std::get<BTBoolean>(*this);
			} else if (std::holds_alternative<BTArray>(*this)) {
				nlohmann::json arr = nlohmann::json::array();
				const auto& vec = std::get<BTArray>(*this);
				for (const auto& e : vec) {
					arr.push_back(e->serialize());
				}
				return arr;
			} else if (std::holds_alternative<BTObject>(*this)) {
				nlohmann::json obj = nlohmann::json::object();
				const auto& map = std::get<BTObject>(*this);
				for (const auto& e : map) {
					obj[e.first] = e.second->serialize();
				}
				return obj;
			} else {
				return {};
			}
		}

		static BTValueType deserialize(const nlohmann::json& j) {
			BTValueType result;
			if (j.is_string()) {
				return BTValueType{j.get<BTString>()};
			} else if (j.is_number()) {
				return BTValueType{j.get<BTNumber>()};
			} else if (j.is_boolean()) {
				return BTValueType{j.get<BTBoolean>()};
			} else if (j.is_null()) {
				return BTValueType{};
			} else if (j.is_array()) {
				auto vec = BTArray{};
				for (const auto& e : j) {
					vec.push_back(std::make_shared<BTValueType>(deserialize(e)));
				}
				return vec;
			} else if (j.is_object()) {
				auto map = BTObject{};
				for (auto& [key, val] : j.items()) {
					map[key] = std::make_shared<BTValueType>(deserialize(val));
				}
				return map;
			} else {
				return BTValueType{};
			}
		}
	};

	struct BTStorage {
		std::unordered_map<std::string, BTValueType> data{};
	};

	struct BTContext {
		Agent entity{};
		BTStorage storage{};
	};

	namespace BTF {
		using Args = BTObject;
		using Command = std::function<BTState(BTContext&, Args)>;

		// Function for safe keyword argument lookup
		template<typename T>
		T getArg(const Args& args, const std::string& key, T defaultValue) {
			if (const auto iterator = args.find(key); iterator != args.end()) {
				if (std::holds_alternative<T>(*iterator->second)) {
					return std::get<T>(*iterator->second);
				}
			}
			return defaultValue;
		}

		template<typename T>
		bool isArg(const Args& args, const std::string& key) {
			if (const auto iterator = args.find(key); iterator != args.end()) {
				return std::holds_alternative<T>(*iterator->second);
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
			if (input.starts_with("*")) {
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

		template <typename T>
		BTState store(BTContext& context, const Args& args, const T& value) {
			const auto storage = BTF::getArg<std::string>(args, "store", "ans");
			context.storage.data[storage] = static_cast<T>(value);
			return BTState::Success;
		}
	}
}
