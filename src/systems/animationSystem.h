#pragma once
#include "registry.h"

namespace df {

    struct AnimationSystem {
        static void update(Registry* registry, float deltaTime);
    };

}
