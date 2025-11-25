#pragma once
#include "animations.h"

namespace df {

    struct AnimationComponent {
        Animation anim;
        std::string currentFrame() const { return anim.getCurrentFrame(); }
    };

}