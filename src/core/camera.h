#pragma once

#include <glm/glm.hpp>

namespace df {

    struct Camera {
        glm::vec2 position = { 0.0f, 0.0f };
        float zoom = 1.0f;
        bool isActive = false;
        float scrollSpeed = 1000000000.0f;
    };

}
