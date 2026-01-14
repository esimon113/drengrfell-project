#include "behaviorTree.h"
using json = nlohmann::json;

void BTSequence::init(const Agent a) {
	this->currentChildIndex[a] = 0;
	for (const auto &child : children) {
		child->init(a);
	}
}

BTState BTSequence::process(const Agent a) {
	for (unsigned i = this->currentChildIndex[a]; i < children.size(); i++) {
		std::shared_ptr<BTNode>& child = children[i];
		switch (child->process(a)) {
			case BTState::Running:
				this->currentChildIndex[a] = i;
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



void BTSelector::init(const Agent a) {
	this->currentChildIndex[a] = 0;
	for (const auto &child : children) {
		child->init(a);
	}
}

BTState BTSelector::process(const Agent a) {
	for (unsigned i = this->currentChildIndex[a]; i < children.size(); i++) {
		std::shared_ptr<BTNode>& child = children[i];
		switch (child->process(a)) {
			case BTState::Running:
				this->currentChildIndex[a] = i;
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


void BTInverter::init(const Agent a) {
	child->init(a);
}

BTState BTInverter::process(const Agent a) {
	switch (child->process(a)) {
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


void BTSucceeder::init(const Agent a) {
	child->init(a);
}

BTState BTSucceeder::process(const Agent a) {
	child->process(a);
	return BTState::Success;
}

nlohmann::json BTSucceeder::serialize() const {
	return {
		{"kind", "succeeder"},
		{"child", child->serialize()}
	};
}


void BTUntilFailureRepeater::init(const Agent a) {
	child->init(a);
}

BTState BTUntilFailureRepeater::process(const Agent a) {
	switch (child->process(a)) {
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


void BTRepeater::init(const Agent a) {
	counter[a] = 0;
	child->init(a);
}

BTState BTRepeater::process(const Agent a) {
	switch (child->process(a)) {
		case BTState::Running:
			return BTState::Running;
		case BTState::Success:
			counter[a]++;
			return counter[a] >= times ? BTState::Success : BTState::Running;
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


void BTFunction::init(const Agent) {}

BTState BTFunction::process(const Agent a) {
	return fn(a);
}

nlohmann::json BTFunction::serialize() const {
	return {
		{"kind", "function"},
		{"name", name}
	};
}

