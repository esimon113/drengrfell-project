#pragma once
#include "glm/vec2.hpp"
#include "gamestate.h"
#include "result.h"

/*
 * This header provides common functionality for render systems.
 */

namespace df {
    struct Viewport {
        glm::uvec2 origin;
        glm::uvec2 size;
    };

    struct RenderError {
        enum class Kind {
            WindowIsNull,
            RegistryIsNull,
            PlayerIsNull,
        };

        RenderError(Kind kind, std::string text);

        Kind kind;
        std::string text;
    };

    glm::vec2 screenToWorldCoordinates(const glm::vec2& screenPos, Viewport viewport) noexcept;
    glm::vec2 calculateWorldDimensions(int columns = 10.0f, int rows = 10.0f) noexcept;
}