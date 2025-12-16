#pragma once
#include <string>

namespace df {

    enum class TutorialStepId {
        MOVE_CAMERA,
        BUILD_SETTLEMENT,
        BUILD_ROAD,
        END
    };

    struct TutorialStep {
        TutorialStepId id;
        std::string text;
        bool completed = false;
        std::optional<glm::vec2> screenPosition;
        bool renderBox = true;
    };

}
