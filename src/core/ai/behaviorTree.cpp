#include "fmt/base.h"
#include <utility>
#include <fstream>

#include "behaviorTree.h"
#include "commandRegistry.h"
#include "jsonUtils.h"

using json = nlohmann::json;

namespace df {
	std::shared_ptr<BTNode> BTNode::deserialize(const json &j, const CommandRegistry& c) {
		std::string kind = j.value("kind", "");
		if (kind == "sequence" || kind == "and") {
			auto ptr = std::make_shared<BTSequence>();
			if (ptr->deserializeInplace(j, c)) return ptr;
		} else if (kind == "selector" || kind == "or") {
			auto ptr = std::make_shared<BTSelector>();
			if (ptr->deserializeInplace(j, c)) return ptr;
		} else if (kind == "inverter" || kind == "not") {
			auto ptr = std::make_shared<BTInverter>();
			if (ptr->deserializeInplace(j, c)) return ptr;
		} else if (kind == "succeeder") {
			auto ptr = std::make_shared<BTSucceeder>();
			if (ptr->deserializeInplace(j, c)) return ptr;
		} else if (kind == "untilFailureRepeater") {
			auto ptr = std::make_shared<BTUntilFailureRepeater>();
			if (ptr->deserializeInplace(j, c)) return ptr;
		} else if (kind == "repeater") {
			auto ptr = std::make_shared<BTRepeater>();
			if (ptr->deserializeInplace(j, c)) return ptr;
		} else if (kind == "function") {
			auto ptr = std::make_shared<BTFunction>();
			if (ptr->deserializeInplace(j, c)) return ptr;
		}
		return nullptr;
	}

	Result<std::shared_ptr<BTNode>, ResultError> BTNode::deserialize(const CommandRegistry& cr, assets::JsonFile asset) {
		auto path = assets::getAssetPath(asset);
		std::ifstream file(path);
		if (!file) {
			return Err(ResultError(ResultError::Kind::IOError, "BTNode::deserialize(): Could not open file: " + path));
		}

		try {
			json j;
			file >> j;
			return Ok(deserialize(j, cr));
		} catch (const json::parse_error& e) {
			return Err(ResultError(ResultError::Kind::JsonParseError, "BTNode::deserialize(): Could not parse file: " + path + ". Reason: " + std::string(e.what())));
		}
	}


	void BTSequence::init(const Agent a) {
		this->currentChildIndex[a] = 0;
		for (const auto &child : children) {
			child->init(a);
		}
	}

	BTState BTSequence::process(BTContext& context) {
		for (unsigned i = this->currentChildIndex[context.entity]; i < children.size(); i++) {
			std::shared_ptr<BTNode>& child = children[i];
			switch (child->process(context)) {
				case BTState::Running:
					this->currentChildIndex[context.entity] = i;
					return BTState::Running;
				case BTState::Success:
					continue;
				case BTState::Failed:
					return BTState::Failed;
				default:
					return BTState::Invalid;
			}
		}
		return BTState::Success;
	}

	nlohmann::json BTSequence::serialize() const {
		nlohmann::json a = nlohmann::json::array();
		for (const auto &child : children) {
			a.emplace_back(child->serialize());
		}
		return {
				{"kind", "sequence"},
				{"children", a}
		};
	}

	bool BTSequence::deserializeInplace(const nlohmann::json& j, const CommandRegistry& cr) {
		nlohmann::json a = j.value("children", nlohmann::json::array());
		if (!a.is_array()) return false; // TODO: Add proper error handling
		for (auto& element : a) {
			auto c = deserialize(element, cr);
			if (c == nullptr) return false;
			this->children.push_back(c);
		}
		return true;
	}


	void BTSelector::init(const Agent a) {
		this->currentChildIndex[a] = 0;
		for (const auto &child : children) {
			child->init(a);
		}
	}

	BTState BTSelector::process(BTContext& context) {
		for (unsigned i = this->currentChildIndex[context.entity]; i < children.size(); i++) {
			std::shared_ptr<BTNode>& child = children[i];
			switch (child->process(context)) {
				case BTState::Running:
					this->currentChildIndex[context.entity] = i;
					return BTState::Running;
				case BTState::Success:
					return BTState::Success;
				case BTState::Failed:
					continue;
				default:
					return BTState::Invalid;
			}
		}
		return BTState::Failed;
	}

	nlohmann::json BTSelector::serialize() const {
		nlohmann::json a = nlohmann::json::array();
		for (const auto &child : children) {
			a.emplace_back(child->serialize());
		}
		return {
				{"kind", "selector"},
				{"children", a}
		};
	}

	bool BTSelector::deserializeInplace(const nlohmann::json& j, const CommandRegistry& cr) {
		nlohmann::json a = j.value("children", nlohmann::json::array());
		if (!a.is_array()) return false; // TODO: Add proper error handling
		for (auto& element : a) {
			auto c = deserialize(element, cr);
			if (c == nullptr) return false;
			this->children.push_back(c);
		}
		return true;
	}


	void BTInverter::init(const Agent a) {
		child->init(a);
	}

	BTState BTInverter::process(BTContext& context) {
		switch (child->process(context)) {
			case BTState::Running:
				return BTState::Running;
			case BTState::Success:
				return BTState::Failed;
			case BTState::Failed:
				return BTState::Success;
			default:
				return BTState::Invalid;
		}
	}

	nlohmann::json BTInverter::serialize() const {
		return {
				{"kind", "inverter"},
				{"child", child->serialize()}
		};
	}

	bool BTInverter::deserializeInplace(const nlohmann::json& j, const CommandRegistry& cr) {
		nlohmann::json o = j.value("child", nlohmann::json::object());
		if (!o.is_object()) return false; // TODO: Add proper error handling
		auto c = deserialize(o, cr);
		if (c == nullptr) return false;
		this->child = c;
		return true;
	}


	void BTSucceeder::init(const Agent a) {
		child->init(a);
	}

	BTState BTSucceeder::process(BTContext& context) {
		child->process(context);
		return BTState::Success;
	}

	nlohmann::json BTSucceeder::serialize() const {
		return {
				{"kind", "succeeder"},
				{"child", child->serialize()}
		};
	}

	bool BTSucceeder::deserializeInplace(const nlohmann::json& j, const CommandRegistry& cr) {
		nlohmann::json o = j.value("child", nlohmann::json::object());
		if (!o.is_object()) return false; // TODO: Add proper error handling
		auto c = deserialize(o, cr);
		if (c == nullptr) return false;
		this->child = c;
		return true;
	}


	void BTUntilFailureRepeater::init(const Agent a) {
		child->init(a);
	}

	BTState BTUntilFailureRepeater::process(BTContext& context) {
		switch (child->process(context)) {
			case BTState::Running:
				return BTState::Running;
			case BTState::Success:
				return BTState::Running;
			case BTState::Failed:
				return BTState::Success;
			default:
				return BTState::Invalid;
		}
	}

	nlohmann::json BTUntilFailureRepeater::serialize() const {
		return {
				{"kind", "untilFailureRepeater"},
				{"child", child->serialize()}
		};
	}

	bool BTUntilFailureRepeater::deserializeInplace(const nlohmann::json& j, const CommandRegistry& cr) {
		nlohmann::json o = j.value("child", nlohmann::json::object());
		if (!o.is_object()) return false; // TODO: Add proper error handling
		auto c = deserialize(o, cr);
		if (c == nullptr) return false;
		this->child = c;
		return true;
	}


	void BTRepeater::init(const Agent a) {
		counter[a] = 0;
		child->init(a);
	}

	BTState BTRepeater::process(BTContext& context) {
		switch (child->process(context)) {
			case BTState::Running:
				return BTState::Running;
			case BTState::Success:
				counter[context.entity]++;
				return counter[context.entity] >= times ? BTState::Success : BTState::Running;
			case BTState::Failed:
				return BTState::Failed;
			default:
				return BTState::Invalid;
		}
	}

	nlohmann::json BTRepeater::serialize() const {
		return {
				{"kind", "repeater"},
				{"times", times},
				{"child", child->serialize()}
		};
	}

	bool BTRepeater::deserializeInplace(const nlohmann::json& j, const CommandRegistry& cr) {
		nlohmann::json o = j.value("child", nlohmann::json::object());
		if (!o.is_object()) return false; // TODO: Add proper error handling
		auto c = deserialize(o, cr);
		if (c == nullptr) return false;
		this->child = c;
		this->times = j.value("times", 1);
		return true;
	}


	BTFunction::BTFunction(std::string name, const CommandRegistry& commandRegistry) : name(std::move(name)) {
		if (!commandRegistry.hasCommand(this->name)) {
			fmt::println(stderr, "[AI Error]: Unknown command '{}'", this->name);
		}
		this->fn = commandRegistry.getCommand(this->name);
	}

	void BTFunction::init(const Agent) {}

	BTState BTFunction::process(BTContext& context) {
		BTF::Args evaluatedArgs = this->args;
		for (auto const& [key, value] : evaluatedArgs) {
			if (std::holds_alternative<std::string>(*value)) {
				evaluatedArgs[key] = std::make_shared<BTValueType>(BTF::evaluateString(context, std::get<std::string>(*value)));
			}
		}
		return this->fn(context, evaluatedArgs);
	}

	nlohmann::json BTFunction::serialize() const {
		return {
				{"kind", "function"},
				{"name", this->name},
				{"args", BTValueType{args}.serialize()}
		};
	}

	bool BTFunction::deserializeInplace(const nlohmann::json& j, const CommandRegistry& commandRegistry) {
		this->name = j.value("name", "");
		if (!commandRegistry.hasCommand(this->name)) return false;
		this->fn = commandRegistry.getCommand(this->name);
		this->args = std::get<BTF::Args>(BTValueType::deserialize(j.value("args", nlohmann::json::object())));
		return true;
	}
}
