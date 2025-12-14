#include "renderCommon.h"

namespace df {
    RenderError::RenderError(const Kind kind, std::string text) : kind(kind), text(std::move(text)) {
    }

    std::ostream &operator<<(std::ostream &out, const RenderError &e) {
        std::string kind;
        switch (e.kind) {
            case RenderError::Kind::NullPointer:
                kind = "NullPointer";
                break;
            case RenderError::Kind::InvalidArgument:
                kind = "InvalidArgument";
                break;
            case RenderError::Kind::DomainError:
                kind = "DomainError";
                break;
        }

        out << kind << ": " << e.text;
        return out;
    }

    glm::vec2 screenToWorldCoordinates(const glm::vec2& screenPos, Viewport viewport) noexcept {
        const glm::vec2 worldDimensions = calculateWorldDimensions(10, 10);

        glm::vec2 viewportPos = screenPos - glm::vec2(viewport.origin);
        glm::vec2 normalizedPos = viewportPos / glm::vec2(viewport.size);
        normalizedPos.y = 1.0f - normalizedPos.y; // flip y: screen-y increases downwards, world-y up

        return normalizedPos * worldDimensions;
    }

    glm::vec2 calculateWorldDimensions(const int columns, const int rows) noexcept {
        // 1,732050808 is sqrt(3)
        return {
            1.732050808f * (columns + 0.5f),
            1.5f * (rows + 1.0f)
        };
    }
}
