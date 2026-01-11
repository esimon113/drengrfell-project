#pragma once
#include "tiny_ecs.hpp"

enum class BTState {
	Running,
	Success,
	Failed,
};

class BTNode {
public:
	virtual ~BTNode() = default;

	virtual void init(Entity) {};
	virtual BTState process(Entity) = 0;
};

class BTIfCondition final : public BTNode {
public:
	BTIfCondition(BTNode* child) : m_child(child) {}
	void init(const Entity e) override {
		m_child->init(e);
	}
	BTState process(Entity e) override {
		if (registry.motions.has(e)) {
			return m_child->process(e);
		} else {
			return BTState::Success;
		}
	}
private:
	BTNode* m_child;
};
