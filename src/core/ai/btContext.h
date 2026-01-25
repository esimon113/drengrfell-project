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

	struct BTMemory {
		std::unordered_map<std::string, std::any> data{};
	};

	struct BTContext {
		Agent entity{};
		BTMemory memory{};
	};

	namespace BTF {
		using JsonType = std::variant<std::string, double, bool>;
		using Args = std::unordered_map<std::string, JsonType>;
		using Command = std::function<BTState(BTContext, Args)>;
	}
}
