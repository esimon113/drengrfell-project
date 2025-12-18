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

        void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;
    private:
        Registry* registry = nullptr;
        Window* window = nullptr;
        GameState* gameState = nullptr;

        Shader tileShader{};
        Shader tilePickerShader{};
        GLuint tileVao = 0;
        GLuint tileVbo = 0;
        GLuint hexVao = 0;
        GLuint hexVbo = 0;
        GLuint tileInstanceVbo = 0;
        TextureArray tileAtlas{};

        struct TileVertex {
            glm::vec2 position;
            glm::vec2 uv;
        };

        struct TileInstance {
            glm::vec2 position;
            std::int32_t type;
            std::int32_t padding;
            std::int32_t explored; // 0 = unexplored, 1 = explored
            std::uint32_t index; // used for mouse picking
        };

        std::vector<TileVertex> tileMesh;
        std::vector<TileVertex> hexMesh;
        std::vector<TileInstance> tileInstances;
        size_t tileInstancesBufferSize = 0;
        unsigned tileColumns = 0;
        unsigned tileRows = 0;

        static std::vector<TileVertex> createHexagonalTileMesh() noexcept;
        static std::vector<TileVertex> createRectangularTileMesh() noexcept;
        void initMap() noexcept;
        void initVao(GLuint vao, GLuint vbo, const std::vector<TileVertex>& mesh) noexcept;
        void renderMap(float timeInSeconds = 0.0) const noexcept;

        Result<std::vector<TileInstance>, ResultError> makeTileInstances(const std::vector<Tile>& tiles, int columns, const Player* player = nullptr) const noexcept;

        bool renderFogOfWar = true;
        bool updateRequired = false;
        bool useHex = false;
    };
}
