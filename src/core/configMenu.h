#pragma once

#include "common.h"

#include "window.h"
#include "utils/shader.h"
#include "utils/texture.h"
#include "types.h"
#include "registry.h"
#include "renderText.h"
#include "worldGeneratorConfig.h"

namespace df {

    class ConfigMenu {
    public:
        ConfigMenu() = default;
        ~ConfigMenu() = default;

        void init(Window* window, Registry* registryParam) noexcept;

        void deinit() noexcept;

        void update(float delta) noexcept;
        void render() noexcept;
        void calcLayout() noexcept;

        void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) noexcept;
        void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;
        void onResizeCallback(GLFWwindow* window, int width, int height) noexcept;

        void setStartCallback(std::function<void(int, int, int, WorldGeneratorConfig::GenerationMode)> callback) {
            onStart = std::move(callback);
        }

        //void setInsularCallback(std::function<void()> callback) { onInsular = std::move(callback); }
        //void setPerlinCallback(std::function<void()> callback) { onPerlin = std::move(callback); }

    private:
        Window* window;
        Registry* registry;

        enum class InputField {
            NONE,
            SEED,
            WIDTH,
            HEIGHT
        };
        InputField activeInput = InputField::NONE;
        std::string inputString;
        int worldSeed = 0;
        int worldHeight = 20;
        int worldWidth = 20;
        WorldGeneratorConfig::GenerationMode worldGenerationMode = WorldGeneratorConfig::GenerationMode::PERLIN;


        Shader menuShader;
        Texture backgroundTexture;
        Texture titleTexture;
        Texture aiBtnTexture;
        Texture easyBtnTexture;
        Texture hardBtnTexture;
        Texture heightBtnTexture;
        Texture insularBtnTexture;
        Texture mediumBtnTexture;
        Texture multiplayerBtnTexture;
        Texture perlinBtnTexture;
        Texture seedBtnTexture;
        Texture startBtnTexture;
        Texture widthBtnTexture;

        GLuint quad_vao;
        GLuint quad_vbo;
        GLuint quad_ebo;

        enum class ButtonId {
            //AI,
            //EASY,
            //HARD,
            HEIGHT,
            INSULAR,
            //MEDIUM,
            //MULTIPLAYER,
            PERLIN,
            SEED,
            START,
            WIDTH,
            count

        };

        struct Button {
            // rectangle in pixels; origin is bottom-left
            float x, y, w, h;
            bool hovered = false;
            //bool enabled = false;
            Texture* texture = nullptr;
            std::function<void()> onClick = nullptr;
        };
        //std::array<Button, static_cast<size_t>(ButtonId::count)> buttons;

        // config menu elements
        
        //Button aiButton{};
        //Button easyButton{};
        //Button hardButton{};
        Button heightButton{};
        Button insularButton{};
        //Button mediumButton{};
        //Button multiplayerButton{};
        Button perlinButton{};
        Button seedButton{};
        Button startButton{};
        Button widthButton{};
        glm::vec2 titlePos{}; // bottom-left of title quad
        glm::vec2 titleSize{};

        //std::function<void()> onAI;
        //std::function<void()> onEasy;
        //std::function<void()> onHard;
        //std::function<void()> onHeight;
        //std::function<void()> onInsular;
        //std::function<void()> onMedium;
        //std::function<void()> onMultiplayer;
        //std::function<void()> onPerlin;
        //std::function<void()> onSeed;
        std::function<void(int, int, int, WorldGeneratorConfig::GenerationMode)> onStart;
        //std::function<void()> onWidth;

        // helper functions
        void capInputValue() noexcept;
        bool isCursorOnButton(float px, float py, const Button& b) const noexcept;
        void initQuadBuffers() noexcept;
    };
}
