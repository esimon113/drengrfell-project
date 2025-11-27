#include "animationSystem.h"

namespace df {

    void AnimationSystem::update(Registry* registry, float deltaTime) {
        for (std::size_t i = 0; i < registry->animations.size(); ++i) {
            //Entity e = registry->animations.entities[i];
            AnimationComponent& animComp = registry->animations.components[i];
            animComp.anim.step(deltaTime); 
        }
    }

}
