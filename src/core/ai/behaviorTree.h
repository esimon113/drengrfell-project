#pragma once
#include "tiny_ecs.hpp"
#include <utility>
#include <nlohmann/json.hpp>

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
	virtual nlohmann::json serialize() const = 0;
	static std::shared_ptr<BTNode> deserialize(const nlohmann::json&);
	virtual bool deserializeInplace(const nlohmann::json&) = 0;
};


class BTSequence final : public BTNode {
public:
	BTSequence() = default;
	explicit BTSequence(const std::vector<std::shared_ptr<BTNode>> &children) : children(children) {}
	void init(Agent) override;
	BTState process(Agent) override;
	nlohmann::json serialize() const override;
	bool deserializeInplace(const nlohmann::json&) override;
private:
	std::vector<std::shared_ptr<BTNode>> children;
	std::map<Agent, unsigned> currentChildIndex;
};


class BTSelector final : public BTNode {
public:
	BTSelector() = default;
	explicit BTSelector(const std::vector<std::shared_ptr<BTNode>> &children) : children(children) {}
	void init(Agent) override;
	BTState process(Agent) override;
	nlohmann::json serialize() const override;
	bool deserializeInplace(const nlohmann::json&) override;
private:
	std::vector<std::shared_ptr<BTNode>> children;
	std::map<Agent, unsigned> currentChildIndex;
};


class BTInverter final : public BTNode {
public:
	BTInverter() = default;
	explicit BTInverter(const std::shared_ptr<BTNode> &child) : child(child) {}
	void init(Agent) override;
	BTState process(Agent) override;
	nlohmann::json serialize() const override;
	bool deserializeInplace(const nlohmann::json&) override;
private:
	std::shared_ptr<BTNode> child;
};


class BTSucceeder final : public BTNode {
public:
	BTSucceeder() = default;
	explicit BTSucceeder(const std::shared_ptr<BTNode> &child) : child(child) {}
	void init(Agent) override;
	BTState process(Agent) override;
	nlohmann::json serialize() const override;
	bool deserializeInplace(const nlohmann::json&) override;
private:
	std::shared_ptr<BTNode> child;
};


class BTUntilFailureRepeater final : public BTNode {
public:
	BTUntilFailureRepeater() = default;
	explicit BTUntilFailureRepeater(const std::shared_ptr<BTNode> &child) : child(child) {}
	void init(Agent) override;
	BTState process(Agent) override;
	nlohmann::json serialize() const override;
	bool deserializeInplace(const nlohmann::json&) override;
private:
	std::shared_ptr<BTNode> child;
};


class BTRepeater final : public BTNode {
public:
	BTRepeater() = default;
	explicit BTRepeater(const std::shared_ptr<BTNode> &child, const unsigned times = 1) : child(child), times(times) {}
	void init(Agent) override;
	BTState process(Agent) override;
	nlohmann::json serialize() const override;
	bool deserializeInplace(const nlohmann::json&) override;
private:
	std::shared_ptr<BTNode> child;
	unsigned times = 1;
	std::map<Agent, unsigned> counter;
};


class BTFunction final : public BTNode {
public:
	using Function = std::function<BTState(Agent)>;
	BTFunction() = default;
	explicit BTFunction(std::string name, Function fn) : name(std::move(name)), fn(std::move(fn)) {}
	void init(Agent) override;
	BTState process(Agent) override;
	nlohmann::json serialize() const override;
	bool deserializeInplace(const nlohmann::json&) override;
private:
	std::string name = "success";
	Function fn = [](Agent){ return BTState::Success; };
};
