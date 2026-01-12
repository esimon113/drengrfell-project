#pragma once
#include "tiny_ecs.hpp"
#include <utility>

/* Assumptions:
 * - The players are the only actors needing an AI
 * - The game is round-based
 */

typedef Entity Agent;

enum class BTState {
	Invalid,
	Running,
	Success,
	Failed,
};


class BTNode {
public:
	virtual ~BTNode() = default;
	virtual void init(Agent) {};
	virtual BTState process(Agent) = 0;
};


class BTSequence final : public BTNode {
public:
	BTSequence() = default;

	explicit BTSequence(const std::vector<std::shared_ptr<BTNode>> &children) : children(children) {}

	void init(const Agent a) override {
		this->currentChildIndex[a] = 0;
		for (const auto &child : children) {
			child->init(a);
		}
	}

	BTState process(const Agent a) override {
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

private:
	std::vector<std::shared_ptr<BTNode>> children;
	std::map<Agent, unsigned> currentChildIndex;
};


class BTSelector final : public BTNode {
public:
	BTSelector() = default;

	explicit BTSelector(const std::vector<std::shared_ptr<BTNode>> &children) : children(children) {}

	void init(const Agent a) override {
		this->currentChildIndex[a] = 0;
		for (const auto &child : children) {
			child->init(a);
		}
	}

	BTState process(const Agent a) override {
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

private:
	std::vector<std::shared_ptr<BTNode>> children;
	std::map<Agent, unsigned> currentChildIndex;
};


class BTInverter final : public BTNode {
public:
	BTInverter() = default;

	explicit BTInverter(const std::shared_ptr<BTNode> &child) : child(child) {}

	void init(const Agent a) override {
		child->init(a);
	}

	BTState process(const Agent a) override {
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

private:
	std::shared_ptr<BTNode> child;
};


class BTSucceeder final : public BTNode {
public:
	BTSucceeder() = default;

	explicit BTSucceeder(const std::shared_ptr<BTNode> &child) : child(child) {}

	void init(const Agent a) override {
		child->init(a);
	}

	BTState process(const Agent a) override {
		child->process(a);
		return BTState::Success;
	}

private:
	std::shared_ptr<BTNode> child;
};


class BTUntilFailureRepeater final : public BTNode {
public:
	BTUntilFailureRepeater() = default;

	explicit BTUntilFailureRepeater(const std::shared_ptr<BTNode> &child) : child(child) {}

	void init(const Agent a) override {
		child->init(a);
	}

	BTState process(const Agent a) override {
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

private:
	std::shared_ptr<BTNode> child;
};


class BTRepeater final : public BTNode {
public:
	BTRepeater() = default;

	explicit BTRepeater(const std::shared_ptr<BTNode> &child, const unsigned times = 1) : child(child), times(times) {}

	void init(const Agent a) override {
		counter[a] = 0;
		child->init(a);
	}

	BTState process(const Agent a) override {
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

private:
	std::shared_ptr<BTNode> child;
	unsigned times = 1;
	std::map<Agent, unsigned> counter;
};


class BTLambda final : public BTNode {
public:
	using Lambda = std::function<BTState(Agent)>;

	BTLambda() = default;

	explicit BTLambda(Lambda lambda) : lambda(std::move(lambda)) {}

	void init(const Agent) override {}

	BTState process(const Agent a) override {
		return lambda(a);
	}
private:
	Lambda lambda = [](Agent){ return BTState::Success; };
};
