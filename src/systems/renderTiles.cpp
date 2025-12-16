#include "renderTiles.h"
#include <iostream>
#include "../core/player.h"
#include "../core/tile.h"
#include "utils/textureArray.h"
#include "common.h"
#include "worldGenerator.h"

namespace df {

    RenderTilesSystem RenderTilesSystem::init(Window& window, Registry& registry, GameState& gameState) noexcept {
        RenderTilesSystem self;

        self.window = &window;
        self.registry = &registry;
        self.gameState = &gameState;

        // load resources for rendering
        self.tileShader = Shader::init(assets::Shader::tile).value();
        self.tileAtlas = TextureArray::init(assets::Texture::TILE_ATLAS);

        self.initMap();

        return self;
    }


    void RenderTilesSystem::initMap() noexcept {
        this->tileMesh = createRectangularTileMesh();

        glGenVertexArrays(1, &tileVao);
        glGenBuffers(1, &tileVbo);
        glGenBuffers(1, &tileInstanceVbo);

        {
            glBindVertexArray(tileVao);
            glBindBuffer(GL_ARRAY_BUFFER, tileVbo);
            glBufferData(GL_ARRAY_BUFFER, this->tileMesh.size() * sizeof(TileVertex), this->tileMesh.data(), GL_STATIC_DRAW);

            // layout(location = 0) in vec2 position;
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, position));

            // layout(location = 1) in vec2 vertexUv;
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, uv));

            // Define instance attributes
            glBindBuffer(GL_ARRAY_BUFFER, tileInstanceVbo);
            glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
            this->tileInstancesBufferSize = 0;

            // layout(location = 2) in vec2 instancePosition;
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TileInstance), (void*)offsetof(TileInstance, position));
            glVertexAttribDivisor(2, 1);

            // layout(location = 3) in int type;
            glEnableVertexAttribArray(3);
            glVertexAttribIPointer(3, 1, GL_INT, sizeof(TileInstance), (void*)offsetof(TileInstance, type));
            glVertexAttribDivisor(3, 1);

            // layout(location = 4) in int explored;
            glEnableVertexAttribArray(4);
            glVertexAttribIPointer(4, 1, GL_INT, sizeof(TileInstance), (void*)offsetof(TileInstance, explored));
            glVertexAttribDivisor(4, 1);

            glBindVertexArray(0);
        }
    }


    Result<void, ResultError> RenderTilesSystem::updateMap() noexcept {
        fmt::print("RenderTilesSystem::updateMap()\n");

        const Player* player = this->gameState->getPlayer(0);
        const Graph& map = this->gameState->getMap();

        const unsigned width = map.getMapWidth();
        const size_t tileCount = map.getTileCount();

        if (width == 0 or width > 100) {
            return Err(ResultError(ResultError::Kind::DomainError, fmt::format("RenderTilesSystem::updateMap() width={} of map({}) is not in [1, 100]. Cannot allocate render buffer.", width, reinterpret_cast<uintptr_t>(&map))));
        }
        if (tileCount == 0 or tileCount > 10000) {
            return Err(ResultError(ResultError::Kind::DomainError, fmt::format("RenderTilesSystem::updateMap() tileCount={} of map({}) is not in [1, 10'000]. Cannot allocate render buffer.", tileCount, reinterpret_cast<uintptr_t>(&map))));
        }
        if (tileCount < width || tileCount % width != 0) {
            return Err(ResultError(ResultError::Kind::DomainError, fmt::format("RenderTilesSystem::updateMap() width and tileCount are inconsistent. Cannot allocate render buffer.")));
        }

        this->tileColumns = map.getMapWidth();
        this->tileRows = map.getTileCount() / this->tileColumns;
        this->tileInstances = makeTileInstances(map.getTiles(), static_cast<int>(this->tileColumns), player);

        const size_t newTileInstancesBufferSize = this->tileInstances.size() * sizeof(TileInstance);
        glBindBuffer(GL_ARRAY_BUFFER, this->tileInstanceVbo);
        if (newTileInstancesBufferSize > this->tileInstancesBufferSize) {
            glBufferData(GL_ARRAY_BUFFER, newTileInstancesBufferSize, this->tileInstances.data(), GL_DYNAMIC_DRAW);
            this->tileInstancesBufferSize = newTileInstancesBufferSize;
        } else {
            glBufferSubData(GL_ARRAY_BUFFER, 0, newTileInstancesBufferSize, this->tileInstances.data());
        }


        return Ok();
    }


    void RenderTilesSystem::deinit() noexcept {
        tileShader.deinit();
        tileAtlas.deinit();

        glDeleteBuffers(1, &tileInstanceVbo);
        glDeleteBuffers(1, &tileVbo);
        glDeleteVertexArrays(1, &tileVao);
        tileInstanceVbo = tileVbo = tileVao = 0;
    }


    float accumulator = 0.0f;
    void RenderTilesSystem::step(const float delta) noexcept {
        accumulator += delta;
        if (accumulator > 1.0) {
            accumulator = 0.0f;
        }
        if (Graph& map = this->gameState->getMap(); map.isRenderUpdateRequested()) {
            if (const Result<void, ResultError> result = updateMap(); result.isErr()) {
                std::cerr << result.unwrapErr() << std::endl;
            }
            map.setRenderUpdateRequested(false);
        }
        renderMap(accumulator);
    }


    void RenderTilesSystem::renderMap(const float timeInSeconds) const noexcept {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const glm::vec2 worldDimensions = calculateWorldDimensions(this->tileColumns, this->tileRows);

        Camera& cam = registry->cameras.get(registry->getCamera());
        glm::vec2 camPos = cam.position;
        float camZoom = cam.zoom;

        const glm::mat4 projection = glm::ortho(
            camPos.x, camPos.x + worldDimensions.x / camZoom,
            camPos.y, camPos.y + worldDimensions.y / camZoom,
            -1.0f, 1.0f
        );

        glm::mat4 model = glm::identity<glm::mat4>();
        model = glm::translate(model, glm::vec3(-camPos, 0.0f));
        model = glm::scale(model, glm::vec3(glm::vec2{1.0f, 1.0f}, 1));

        tileAtlas.bind(0);
        tileShader.use()
            .setMat4("model", model)
            .setMat4("projection", projection)
            .setFloat("time", timeInSeconds)
            .setInt("frames", 4)
            .setSampler("tileAtlas", 0);

        glBindVertexArray(tileVao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, static_cast<GLsizei>(this->tileMesh.size()), static_cast<GLsizei>(this->tileInstances.size()));
        glBindVertexArray(0);
    }


    void RenderTilesSystem::reset() noexcept {}


    std::vector<RenderTilesSystem::TileVertex> RenderTilesSystem::createRectangularTileMesh() noexcept {
        std::vector<TileVertex> vertices;
        vertices.push_back({{1, -1}, {1, 0}});
        vertices.push_back({{1, 1}, {1, 1}});
        vertices.push_back({{-1, 1}, {0, 1}});
        vertices.push_back({{-1, -1}, {0, 0}});

        std::vector<TileVertex> meshData;
        for (int i = 0; i < 2; i++) {
            meshData.push_back(vertices[2 * i + 0]);
            meshData.push_back(vertices[2 * i + 1]);
            meshData.push_back(vertices[(2 * i + 2) % 4]);
        }

        return meshData;
    }


    std::vector<RenderTilesSystem::TileInstance> RenderTilesSystem::makeTileInstances(const std::vector<Tile>& tiles, const int columns, const Player* player) noexcept {
        const int rows = static_cast<int>(tiles.size()) / columns;
        std::vector<TileInstance> instances;

        // The iteration order is important!
        for (int row = rows - 1; row >= 0; row--) {
            for (int column = 0; column < columns; column++) {
                glm::vec2 position;
                position.x = 2.0f * (static_cast<float>(column) + 0.5f * static_cast<float>(row & 1));
                position.y = static_cast<float>(row) * 1.5f;

                const Tile& tile = tiles[row * columns + column];
                instances.push_back({position, static_cast<int>(tile.getType()), 0, player == nullptr});
            }
        }

        // If no player is given, the whole map is shown as explored
        if (player != nullptr) {
            // Do not move into double loop for performance reasons [O(n^2)+O(n) vs. O(n^3)]
            for (const size_t tileId : player->getExploredTileIds()) {
                if (tileId < instances.size()) {
                    instances[tileId].explored = 1;
                }
            }
        }

        return instances;
    }
}
