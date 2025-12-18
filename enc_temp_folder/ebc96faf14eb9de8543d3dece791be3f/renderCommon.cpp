#include "renderCommon.h"

namespace df {
    glm::vec2 screenToWorldCoordinates(const glm::vec2& screenPos, const Viewport viewport, const glm::vec2 worldDimensions) noexcept {
        const glm::vec2 viewportPos = screenPos - glm::vec2(viewport.origin);
        glm::vec2 normalizedPos = viewportPos / glm::vec2(viewport.size);
        normalizedPos.y = 1.0f - normalizedPos.y; // flip y: screen-y increases downwards, world-y up

        return normalizedPos * worldDimensions;
    }

    glm::vec2 calculateWorldDimensions(const int columns, const int rows) noexcept {
        float hexWidth = 2.f;   // passt zum Rendering
        float hexHeight = 2.f * 3.f / 4.f;  // Höhe für pointy-topped: 3/4 * width

        return {
            hexWidth * (columns + 0.5f),
            hexHeight * (rows + 1.f)
        };
    }
}
