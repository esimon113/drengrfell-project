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

        static RenderTilesSystem init(Window& window, Registry& registry, GameState& gameState) noexcept;
        void deinit() noexcept;

        void step(float delta) noexcept;
        void reset() noexcept;

        // Call this only if map size has changed. Everything else is handled in step()
        [[nodiscard]] Result<void, ResultError> updateMap() noexcept;
    private:
        Registry* registry;
        Window* window;
        GameState* gameState;

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
        size_t tileInstancesBufferSize;
        unsigned tileColumns;
        unsigned tileRows;

        static std::vector<float> createRectangularTileMesh() noexcept;
        void initMap() noexcept;
        void renderMap(float timeInSeconds = 0.0) const noexcept;

        static std::vector<TileInstance> makeTileInstances(const std::vector<Tile>& tiles, int columns, const Player* player = nullptr) noexcept;
    };
}
