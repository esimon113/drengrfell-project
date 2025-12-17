#pragma once

#include <map>
#include <string>

#include <window.h>
#include <registry.h>
#include <utils/shader.h>
#include "renderCommon.h" 


namespace df {

    class RenderTextSystem {
    public:
        RenderTextSystem() = default;

        static RenderTextSystem init(Window* window, Registry* registry) noexcept;
        void deinit() noexcept;

        void step(float delta) noexcept;
        void reset() noexcept;

        void renderText(
            std::string text,
            glm::vec2 position,
            float scale,
            glm::vec3 color
        ) const noexcept;

        Viewport getViewport() const noexcept;
        void updateViewport(glm::uvec2 origin, glm::uvec2 size) {
            this->viewport.origin = origin;
            this->viewport.size = size;
        }

    private:
        struct Character {
            GLuint textureID;
            glm::ivec2 size;
            glm::ivec2 bearing;
            GLuint advance;
        };

        std::map<char, Character> characters;

        Shader textShader;
        GLuint vao = 0;
        GLuint vbo = 0;

        Window* window = nullptr;
        Registry* registry = nullptr;
        Viewport viewport;
    };

}
