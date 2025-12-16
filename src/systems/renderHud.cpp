#include "renderHud.h"
#include <iostream>

namespace df {

    RenderHudSystem RenderHudSystem::init(Window* window, Registry* registry) noexcept {
        RenderHudSystem self;

        self.window = window;
        self.registry = registry;

        self.viewport.origin = glm::uvec2(0);
        self.viewport.size = self.window->getWindowExtent();

        glm::uvec2 extent = self.viewport.size;

        self.rectShader = Shader::init(assets::Shader::hud).value();

        // Viewport
        glViewport(0, 0, extent.x, extent.y);

        // Quad rectangle 1x1, transformed via model later
        float quad[] = {
            0.f, 0.f,
            1.f, 0.f,
            1.f, 1.f,
            0.f, 1.f
        };

        glGenVertexArrays(1, &self.quadVao);
        glBindVertexArray(self.quadVao);

        glGenBuffers(1, &self.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, self.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

        glBindVertexArray(0);
        // TMP to delete: test initilize player
        Entity playerEntity;
        registry->players.emplace(playerEntity);
        for (Entity e : registry->players.entities) {

            std::cout << "innerhalb der step hud schleife 1" << std::endl;
            Player& player = registry->players.get(e);
            player.addResources(types::TileType::FOREST, 10);
            player.addResources(types::TileType::MOUNTAIN, 5);
            player.addResources(types::TileType::FIELD, 8);
        }
        //

        return self;
    }

    void RenderHudSystem::deinit() noexcept {
        rectShader.deinit();
    }

    void RenderHudSystem::step(float /*dt*/) noexcept {
        renderHud();
        RenderTextSystem* textSystem = registry->getSystem<RenderTextSystem>();
        if (textSystem) {
            for (Entity e : registry->players.entities) {

                std::cout << "innerhalb der step hud schleife 2" << std::endl;
                Player& player = registry->players.get(e);
                std::map<types::TileType, int> resources = player.getResources();
                textSystem->renderText(
                    "Wood: " + std::to_string(resources[types::TileType::FOREST]) +
                    "; Stone: " + std::to_string(resources[types::TileType::MOUNTAIN]) +
                    "; Grain: " + std::to_string(resources[types::TileType::FIELD]) +
                    "; Round: 1", // TODO: update current round
                    { 20.0f, 27.0f },
                    0.5f,
                    { 1.0f, 1.0f, 1.0f }
                );
            }
        }
    }

    void RenderHudSystem::reset() noexcept {
        renderHud();
        RenderTextSystem* textSystem = registry->getSystem<RenderTextSystem>();
        if (textSystem) {
            textSystem->renderText(" ", { 0.0f, 0.0f }, 0.5f, { 1.0f, 1.0f, 1.0f });
        }
    }

    void RenderHudSystem::renderHud() const noexcept {

        const float width = viewport.size.x;
        const float height = viewport.size.y;
        
        glm::mat4 projection = glm::ortho(0.f, width, 0.f, height, -1.f, 1.f);

        // Modellmatrix
        glm::vec2 pos = { 10.f, 10.f };
        glm::vec2 size = { 400.f, 50.f };       // Size of the HUD

        glm::mat4 model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(pos, 0.f));
        model = glm::scale(model, glm::vec3(size, 1.f));

        rectShader.use()
            .setMat4("projection", projection)
            .setMat4("view", glm::identity<glm::mat4>())
            .setMat4("model", model)
            .setVec3("fcolor", glm::vec3(0.f, 0.f, 0.f));  

        glBindVertexArray(quadVao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);
    }

}
