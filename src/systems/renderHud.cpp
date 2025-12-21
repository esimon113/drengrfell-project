#include "renderHud.h"
#include <iostream>

namespace df {

    RenderHudSystem RenderHudSystem::init(Window* window, Registry* registry, std::shared_ptr<GameState> gameState) noexcept {
        RenderHudSystem self;

        self.window = window;
        self.registry = registry;
        self.gameState = gameState;

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

        // Hud Settings
        self.hudPos = { window->getWindowExtent().x * 0.02f, window->getWindowExtent().y * 0.02f }; // hud starts 2% right and 2% of the bottom
        self.hudSize = {
            window->getWindowExtent().x * 0.96f,    // 2% space to the right corner
            window->getWindowExtent().y * 0.08f     // 6% in height starting from 2% window height
        };
        // End Turn Button
        float paddingX = self.hudSize.x * 0.02f;    // ensure black hud layout below endTurn button
        self.endTurnButton.w = self.hudSize.x * 0.20f;
        self.endTurnButton.h = self.hudSize.y * 0.7f;
        self.endTurnButton.x = self.hudPos.x + self.hudSize.x - self.endTurnButton.w - paddingX;
        self.endTurnButton.y = self.hudPos.y + (self.hudSize.y - self.endTurnButton.h) / 2.0f;

        return self;
    }

    void RenderHudSystem::deinit() noexcept {
        rectShader.deinit();
    }

    void RenderHudSystem::step(float /*dt*/) noexcept {
        RenderTextSystem* textSystem = registry->getSystem<RenderTextSystem>();
        if (textSystem) {
            // Render Tutorial
            if (gameState->isTutorialActive()) {
                // get next tutorial step
                TutorialStep* step = gameState->getCurrentTutorialStep();
                if (!step)
                    return;
                // box position/size
                float boxPosPaddingX = 10.f;
                float boxPosPaddingY = 10.f;
                float scale = 0.4f;
                glm::vec2 textSize = textSystem->measureText(step->text, scale);
                glm::vec2 rectBoxSize = {
                    textSize.x + boxPosPaddingX,
                    textSize.y + boxPosPaddingY
                };
                glm::vec2 pos = { 10.0f, window->getWindowExtent().y - rectBoxSize.y - 10.0f };     // box position always top left
                // Tutorial box
                if (step->renderBox) {
                    renderRectBox(pos, rectBoxSize, {0.0f, 0.0f, 0.0f});
                }

                // center text in the middle of the box
                float offset = 15.0f;
                glm::vec2 textPos = {
                    pos.x + (rectBoxSize.x - textSize.x) / 2.0f,
                    pos.y + (rectBoxSize.y + textSize.y) / 2.0f - offset
                };
                // Tutorial-Text
                textSystem->renderText(step->text, textPos, scale, { 1.f, 1.f, 1.f });
            }
            // Render HUD
            // TODO: update for multiple player if we do multiplayer
            Player& player = *gameState->getPlayer(0);
            std::map<types::TileType, int> resources = player.getResources();
            std::string hudTextToPrint = "Wood: " + std::to_string(resources[types::TileType::FOREST]) +
                "; Stone: " + std::to_string(resources[types::TileType::MOUNTAIN]) +
                "; Clay: " + std::to_string(resources[types::TileType::CLAY]) +
                "; Grass: " + std::to_string(resources[types::TileType::GRASS]) +
                "; Grain: " + std::to_string(resources[types::TileType::FIELD]) +
                "; Round: " + std::to_string(gameState->getRoundNumber());
            float scale = viewport.size.x * 0.05f / 100.f;  // scale text size for fullscreen
            glm::vec2 hudTextSize = textSystem->measureText(hudTextToPrint, scale);
            glm::vec2 hudTextPos = {
                hudPos.x + 10.0f,
                hudPos.y + (hudSize.y - hudTextSize.y) / 2.0f
            };
            // Render Box for HUD
            renderRectBox(hudPos, hudSize, {0.0f, 0.0f, 0.0f});
            // Render Text for HUD
            textSystem->renderText(
                hudTextToPrint,
                hudTextPos,
                scale,
                { 1.0f, 1.0f, 1.0f }
            );

            // End Turn Button
            renderRectBox({ endTurnButton.x , endTurnButton.y }, { endTurnButton.w, endTurnButton.h }, {0.0f, 0.0f, 1.0f});
            glm::vec2 textSizeEndTurn = textSystem->measureText("End Turn", 0.5f);
            glm::vec2 buttonTextPos = {
                endTurnButton.x + (endTurnButton.w - textSizeEndTurn.x) / 2.0f,
                endTurnButton.y + (endTurnButton.h - textSizeEndTurn.y) / 2.0f
            };
            textSystem->renderText("End Turn", buttonTextPos, 0.5f, { 1.f, 1.f, 1.f });

        }
    }

    void RenderHudSystem::reset() noexcept {
        RenderTextSystem* textSystem = registry->getSystem<RenderTextSystem>();
        if (textSystem) {
            textSystem->renderText(" ", { 0.0f, 0.0f }, 0.5f, { 1.0f, 1.0f, 1.0f });
        }
    }

    void RenderHudSystem::renderRectBox(glm::vec2 pos, glm::vec2 size, glm::vec3 color) const noexcept {

        const float width = viewport.size.x;
        const float height = viewport.size.y;

        glm::mat4 projection = glm::ortho(0.f, width, 0.f, height, -1.f, 1.f);

        glm::mat4 model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(pos, 0.f));
        model = glm::scale(model, glm::vec3(size, 1.f));

        rectShader.use()
            .setMat4("projection", projection)
            .setMat4("view", glm::identity<glm::mat4>())
            .setMat4("model", model)
            .setVec3("fcolor", color);

        glBindVertexArray(quadVao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);
    }

    bool RenderHudSystem::isMouseOverEndTurn(glm::vec2 mouse) const noexcept {
        return mouse.x >= endTurnButton.x &&
            mouse.x <= endTurnButton.x + endTurnButton.w &&
            mouse.y >= endTurnButton.y &&
            mouse.y <= endTurnButton.y + endTurnButton.h;
    }

    bool RenderHudSystem::wasEndTurnClicked(glm::vec2 mouse, int button, int action) const noexcept {
        bool result = button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && isMouseOverEndTurn(mouse);
        return result;
    }
    

    bool RenderHudSystem::onMouseButton(glm::vec2 mouse, int button, int action) noexcept {
        return wasEndTurnClicked(mouse, button, action) ? true : false; // endTurn() will be called by Application
    }

    void RenderHudSystem::onResizeCallback(GLFWwindow* /*window*/, int width, int height) noexcept {
       
        this->viewport.size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        hudSize = {
            window->getWindowExtent().x * 0.96f,    // 2% space to the right corner
            window->getWindowExtent().y * 0.08f     // 6% in height starting from 2% window height
        };
        hudPos = { window->getWindowExtent().x * 0.02f, window->getWindowExtent().y * 0.02f };  // hud starts 2% right and 2% of the bottom
        float paddingX = hudSize.x * 0.02f;         // ensure black hud layout below endTurn button
        endTurnButton.w = hudSize.x * 0.20f; 
        endTurnButton.h = hudSize.y * 0.7f;   
        endTurnButton.x = hudPos.x + hudSize.x - endTurnButton.w - paddingX;
        endTurnButton.y = hudPos.y + (hudSize.y - endTurnButton.h) / 2.0f; 

    }

}
