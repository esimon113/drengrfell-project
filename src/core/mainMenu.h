#pragma once

#include "common.h"

#include "window.h"
#include "utils/shader.h"
#include "utils/texture.h"
#include "types.h"
#include "gamestate.h"

namespace df {

    class MainMenu {
    public:
        MainMenu() = default;
        ~MainMenu() = default;

        void init(Window* window) noexcept;

        void deinit() noexcept;

        void update(float delta) noexcept;
        void render() noexcept;
        void calcLayout() noexcept;

        void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) noexcept;
        void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;
        void onResizeCallback(GLFWwindow* window, int width, int height) noexcept;

        // DEL void setGameState(GameState* state) { this->gameState = state; }
        // DEL GameState* getGameState() {return this->gameState; }

        void setStartCallback(std::function<void()> callback) { onStart = std::move(callback); }
        void setExitCallback(std::function<void()> callback) { onExit = std::move(callback); }

    private:
        Window* window;

        Shader menuShader;
        Texture titleTexture;
        Texture startBtnTexture;
        Texture exitBtnTexture;
        Texture backgroundTexture;

        GLuint quad_vao;
        GLuint quad_vbo;
        GLuint quad_ebo;

        struct Button {
            // rectangle in pixels; origin is bottom-left
            float x, y, w, h;
            bool hovered = false;
        };

        // main menu elements
        Button startButton{};
        Button exitButton{};
        glm::vec2 titlePos{}; // bottom-left of title quad
        glm::vec2 titleSize{};

        // DEL GameState* gameState = nullptr;
        std::function<void()> onStart;
        std::function<void()> onExit;

        // helper functions
        bool isCursorOnButton(float px, float py, const Button& b) const noexcept;
        void initQuadBuffers() noexcept;
    };
}
