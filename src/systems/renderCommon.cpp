#include "renderCommon.h"

namespace df {
    glm::vec2 screenToWorldCoordinates(const glm::vec2& screenPos, const Viewport viewport) noexcept {
        const glm::vec2 worldDimensions = calculateWorldDimensions(10, 10);

        const glm::vec2 viewportPos = screenPos - glm::vec2(viewport.origin);
        glm::vec2 normalizedPos = viewportPos / glm::vec2(viewport.size);
        normalizedPos.y = 1.0f - normalizedPos.y; // flip y: screen-y increases downwards, world-y up

        return normalizedPos * worldDimensions;
    }

    glm::vec2 calculateWorldDimensions(const int columns, const int rows) noexcept {
        // 1,732050808 is sqrt(3)
        return {
            1.732050808f * (static_cast<float>(columns) + 0.5f),
            1.5f * (static_cast<float>(rows) + 1.0f)
        };
    }
}
