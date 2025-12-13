#pragma once

#include "renderCommon.h"
#include <registry.h>
#include <window.h>
#include <utils/shader.h>
#include <utils/framebuffer.h>
#include "utils/textureArray.h"


namespace df {
    class Player;
    class RenderTilesSystem {
    public:
        RenderTilesSystem() = default;
        ~RenderTilesSystem() = default;

        static RenderTilesSystem init(Window* window, Registry* registry) noexcept;
        void deinit() noexcept;

        void step(float delta) noexcept;
        void reset() noexcept;

        void updateFogOfWar(const Player& player) noexcept;
    private:
        Registry* registry;
        Window* window;
        Framebuffer intermediateFramebuffer;

        Shader tileShader;
        GLuint tileVao;
        GLuint tileVbo;
        GLuint tileInstanceVbo;
        TextureArray tileAtlas;

        Viewport viewport;

        struct TileVertex {
            glm::vec2 position;
            glm::vec2 uv;
        };
        static constexpr int FLOATS_PER_TILE_VERTEX = 4;

        struct TileInstance {
            glm::vec2 position;
            int type;
            int padding;
            int explored; // 0 = unexplored, 1 = explored
        };

        std::vector<float> tileMesh;
        std::vector<TileInstance> tileInstances;

        static std::vector<float> createRectangularTileMesh() noexcept;
        static std::vector<TileInstance> createTileInstances(int columns = 10.0f, int rows = 10.0f) noexcept;
        void initMap() noexcept;
        void renderMap(float timeInSeconds = 0.0, glm::vec2 scale = {1.5f, 1.5f}) const noexcept;
    };
}
