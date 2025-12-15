#include "animationSystem.h"

namespace df {

    void AnimationSystem::update(Registry* registry, float deltaTime) {
        for (std::size_t i = 0; i < registry->animations.size(); ++i) {
            AnimationComponent& animComp = registry->animations.components[i];
            animComp.anim.step(deltaTime);
        }
    }
}
