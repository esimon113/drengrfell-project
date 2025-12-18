#include "renderTiles.h"
#include <iostream>
#include "../core/player.h"
#include "../core/tile.h"
#include "utils/textureArray.h"
#include "common.h"
#include "worldGenerator.h"

namespace df {

    RenderTilesSystem RenderTilesSystem::init(Window& window, Registry& registry, std::shared_ptr<GameState> gameState) noexcept {
        RenderTilesSystem self;

        self.window = &window;
        self.registry = &registry;
        self.gameState = gameState;

        const glm::uvec2 extent = self.window->getWindowExtent();
        self.intermediateFramebuffer = Framebuffer::init({ static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), 1, true });

        // load resources for rendering
        self.tileShader = Shader::init(assets::Shader::tile).value();
        self.tilePickerShader = Shader::init(assets::Shader::tilePicker).value();
        self.tileAtlas = TextureArray::init(assets::Texture::TILE_ATLAS);

        self.initMap();

        return self;
    }


    void RenderTilesSystem::initMap() noexcept {
        this->tileMesh = createRectangularTileMesh();
        this->hexMesh = createHexagonalTileMesh();

        glGenVertexArrays(1, &this->tileVao);
        glGenVertexArrays(1, &this->hexVao);
        glGenBuffers(1, &this->tileVbo);
        glGenBuffers(1, &this->hexVbo);
        glGenBuffers(1, &this->tileInstanceVbo);

        glBindBuffer(GL_ARRAY_BUFFER, this->tileInstanceVbo);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        this->tileInstancesBufferSize = 0;

        initVao(this->tileVao, this->tileVbo, this->tileMesh);
        initVao(this->hexVao, this->hexVbo, this->hexMesh);
    }


    void RenderTilesSystem::initVao(const GLuint vao, const GLuint vbo, const std::vector<TileVertex>& mesh) noexcept {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, mesh.size() * sizeof(TileVertex), mesh.data(), GL_STATIC_DRAW);

        // layout(location = 0) in vec2 position;
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, position));

        // layout(location = 1) in vec2 vertexUv;
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, uv));

        // Define instance attributes
        glBindBuffer(GL_ARRAY_BUFFER, this->tileInstanceVbo);

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

        // layout(location = 5) in uint tileIndex;
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 1, GL_UNSIGNED_INT, sizeof(TileInstance), (void*)offsetof(TileInstance, index));
        glVertexAttribDivisor(5, 1);

        glBindVertexArray(0);
    }


    Result<void, ResultError> RenderTilesSystem::updateMap() noexcept {
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

        this->tileColumns = RenderCommon::getMapColumns<unsigned>(map);
        this->tileRows = RenderCommon::getMapRows<unsigned>(map);
        auto tileInstanceResult = makeTileInstances(map.getTiles(), static_cast<int>(this->tileColumns), player);
        if (tileInstanceResult.isOk()) {
            this->tileInstances = tileInstanceResult.unwrap<>();
        } else {
            return Err(tileInstanceResult.unwrapErr());
        }

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


    void RenderTilesSystem::onKeyCallback(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/) noexcept {
        switch (action) {
            case GLFW_PRESS: {
                switch (key) {
                    case GLFW_KEY_F: {
                        this->renderFogOfWar ^= true;
                        this->updateRequired = true;
                        fmt::println("Set fow rendering to {}", this->renderFogOfWar ? "true" : "false");
                    } break;
                    /*case GLFW_KEY_H: {
                        this->useHex ^= true;
                        this->updateRequired = true;
                        fmt::println("Set hex rendering to {}", this->useHex ? "true" : "false");
                    } break;*/
                    case GLFW_KEY_P: {
                        double xpos, ypos;
                        glfwGetCursorPos(this->window->getHandle(), &xpos, &ypos);
                        auto extent = this->window->getWindowExtent();

                        auto tileId = getTileIdAtPosition(xpos, extent.y - ypos);
                        auto mapId = tileIdToMapId(tileId);
                        fmt::println("Picked: TileId {} / MapId {} at mouse ({}, {})", tileId, mapId, xpos, ypos);
                    } break;
                }
            }
        }
    }


    int RenderTilesSystem::tileIdToMapId(unsigned tileId) const noexcept {
        if (tileId == 0) return -1;
        tileId--;

        const auto columns = this->tileColumns;
        const auto rows = this->tileRows;

        if (tileId < rows * columns) {
            const size_t row = tileId / columns;
            const size_t col = tileId % columns;
            const size_t instanceId = (rows - 1 - row) * columns + col;
            if (instanceId < this->tileInstances.size()) {
                return static_cast<int>(instanceId);
            }
        }

        return -1;
    }


    int RenderTilesSystem::getMapIdAtMouse() noexcept {
        double xpos, ypos;
        glfwGetCursorPos(this->window->getHandle(), &xpos, &ypos);
        const auto extent = this->window->getWindowExtent();

        const auto tileId = getTileIdAtPosition(xpos, extent.y - ypos);
        const auto mapId = tileIdToMapId(tileId);

        return mapId;
    }


    void RenderTilesSystem::deinit() noexcept {
        tileShader.deinit();
        tileAtlas.deinit();
        intermediateFramebuffer.deinit();

        glDeleteBuffers(1, &tileInstanceVbo);
        glDeleteBuffers(1, &tileVbo);
        glDeleteBuffers(1, &hexVbo);
        glDeleteVertexArrays(1, &tileVao);
        glDeleteVertexArrays(1, &hexVao);
        tileInstanceVbo = tileVbo = tileVao = hexVbo = hexVao = 0;
    }


    float accumulator = 0.0f;
    void RenderTilesSystem::step(const float delta) noexcept {
        accumulator += delta;
        if (accumulator > 1.0) {
            accumulator = 0.0f;
        }
        if (Graph& map = this->gameState->getMap(); map.isRenderUpdateRequested() or this->updateRequired) {
            if (const Result<void, ResultError> result = updateMap(); result.isErr()) {
                std::cerr << result.unwrapErr() << std::endl;
            }
            map.setRenderUpdateRequested(false);
            this->updateRequired = false;
        }
        renderMap(accumulator);
        //renderPickerMap(true);
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

        this->tileAtlas.bind(0);
        this->tileShader.use()
            .setMat4("model", model)
            .setMat4("projection", projection)
            .setFloat("time", timeInSeconds)
            .setInt("frames", 4)
            .setInt("selectedTile", this->selectedTile)
            .setSampler("tileAtlas", 0);

        glBindVertexArray(useHex ? hexVao : tileVao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, static_cast<GLsizei>((useHex ? this->hexMesh : this->tileMesh).size()), static_cast<GLsizei>(this->tileInstances.size()));
        glBindVertexArray(0);
    }


    void RenderTilesSystem::renderPickerMap(bool blend) const noexcept {
        if (blend) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glDisable(GL_BLEND);
        }

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

        this->tilePickerShader.use()
            .setMat4("model", model)
            .setMat4("projection", projection);

        glBindVertexArray(hexVao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, static_cast<GLsizei>(hexMesh.size()), static_cast<GLsizei>(this->tileInstances.size()));
        glBindVertexArray(0);
    }


    unsigned RenderTilesSystem::getTileIdAtPosition(const int x, const int y) noexcept {
        // See https://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-an-opengl-hack/

        const glm::uvec2 windowExtent = this->window->getWindowExtent();
        const glm::uvec2 framebufferExtent = this->intermediateFramebuffer.getExtent();
        if (windowExtent != framebufferExtent) {
            this->intermediateFramebuffer.deinit();
            this->intermediateFramebuffer = Framebuffer::init({ static_cast<GLsizei>(windowExtent.x), static_cast<GLsizei>(windowExtent.y), 1, true });
        }

        this->intermediateFramebuffer.bind();

        glm::uvec2 extent = this->intermediateFramebuffer.getExtent();
        if (static_cast<unsigned>(x) >= extent.x || static_cast<unsigned>(y) >= extent.y) {
            std::cerr << "Tile picker coordinates are out of bounds: " << "x: " << x << ", y: " << y;
            return 0;
        }
        glViewport(0, 0, extent.x, extent.y);
        glClearColor(0.0, 0.0, 0.0f , 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_PROGRAM_POINT_SIZE);
        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        renderPickerMap(false);

        glFlush();
        glFinish();
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        unsigned char data[4];
        glReadPixels(x, y,1,1, GL_RGBA, GL_UNSIGNED_BYTE, data);

        this->intermediateFramebuffer.unbind();

        return data[0] +
               data[1] * 256 +
               data[2] * 256*256;
    }


    void RenderTilesSystem::reset() noexcept {}


    std::vector<RenderTilesSystem::TileVertex> RenderTilesSystem::createHexagonalTileMesh() noexcept {
        // Appends the hexagons corners counter-clockwise to the vertices array.
        // The center of the hexagon is at the origin.
        // It is rotated by 30 degrees in order to have a corner at the top,
        // as the tile textures already created have also the corner at the top.

        constexpr float SQRT_3_DIV_2 = 0.866025404f; //sqrt(3.0) / 2.0f;
        std::vector<TileVertex> vertices;
        for (int vertex = 0; vertex < 6; vertex++) {
            const float angle = M_PI / 180.0f * (60.0f * static_cast<float>(vertex) - 30.0f);
            const float x = std::cos(angle);
            const float y = std::sin(angle);
            const float u = (x + SQRT_3_DIV_2) / (2.0f * SQRT_3_DIV_2);
            const float v = (y + 1.0f) / 2.0f;
            vertices.emplace_back(glm::vec2(x, y), glm::vec2(u, v));
        }

        // This is a triangulation I've come up with on my ipad.
        // The triangles are counter-clockwise as the vertices above
        // are counter-clockwise around the origin.

        std::vector<TileVertex> meshData;

        // Big center triangle
        for (int i = 0; i < 6; i += 2) {
            meshData.push_back(vertices[i]);
        }

        // Three side triangles
        for (int i = 0; i < 3; i++) {
            meshData.push_back(vertices[2 * i + 0]);
            meshData.push_back(vertices[2 * i + 1]);
            meshData.push_back(vertices[(2 * i + 2) % 6]);
        }

        return meshData;
    }


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


    Result<std::vector<RenderTilesSystem::TileInstance>, ResultError> RenderTilesSystem::makeTileInstances(const std::vector<Tile>& tiles, const int columns, const Player* player) const noexcept {
        if (!this->renderFogOfWar) {
            player = nullptr;
        }

        const int rows = static_cast<int>(tiles.size()) / columns;
        std::vector<TileInstance> instances;

        // For tile picking. 0 = None
        std::uint32_t index = 1;
        // The iteration order is important!
        for (int row = rows - 1; row >= 0; row--) {
            for (int column = 0; column < columns; column++) {
                const glm::vec2 position = RenderCommon::rowColToWorldCoordinates(column, row);
                const Tile& tile = tiles[row * columns + column];
                instances.push_back({position, static_cast<int>(tile.getType()), 0, player == nullptr, index});
                index++;
            }
        }

        // If no player is given, the whole map is shown as explored
        if (player != nullptr) {
            // Do not move into double loop for performance reasons
            for (const size_t tileId : player->getExploredTileIds()) {
                if (static_cast<int>(tileId) < rows * columns) {
                    const size_t row = tileId / columns;
                    const size_t col = tileId % columns;
                    const size_t instanceId = (rows - 1 - row) * columns + col;
                    if (instanceId < instances.size()) {
                        instances[instanceId].explored = 1;
                    } else {
                        return Err(ResultError(ResultError::Kind::DomainError, fmt::format("RenderTilesSystem::makeTileInstance: invalid instanceId={}. There are only {} instances.", instanceId, instances.size())));
                    }
                }
            }
        }

        return Ok(instances);
    }
}
