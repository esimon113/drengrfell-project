#pragma once
#include "registry.h"
#include "renderCommon.h"
#include "window.h"
#include <utils/shader.h>
#include <renderText.h>

namespace df {
    class RenderHudSystem {
    public:
        RenderHudSystem() = default;
        ~RenderHudSystem() = default;

        static RenderHudSystem init(Window* window, Registry* registry, GameState& gameState) noexcept;
        void deinit() noexcept;
        void step(float dt) noexcept;
        void reset() noexcept;
        void renderHud() const noexcept;

        void renderRectBox(glm::vec2 pos, glm::vec2 size, glm::vec3 color) const noexcept;
        bool isMouseOverEndTurn(glm::vec2 mouse) const noexcept;
        bool onMouseButton(glm::vec2 mouse, int button, int action) noexcept;


    private:
        Registry* registry = nullptr;
        Window* window = nullptr;
        GameState* gameState;
        Viewport viewport;

        Shader rectShader;
        GLuint quadVao = 0;
        GLuint vbo = 0; 

        struct Button {
            // rectangle in pixels; origin is bottom-left
            float x, y, w, h;
        };

        Button endTurnButton{};
    };
}
