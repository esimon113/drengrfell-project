#include "renderHud.h"
#include <iostream>

namespace df {

    RenderHudSystem RenderHudSystem::init(Window* window, Registry* registry, GameState& gameState) noexcept {
        RenderHudSystem self;

        self.window = window;
        self.registry = registry;
        self.gameState = &gameState;

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

        // End Turn Button
        float buttonWidth = 0.20f;
        float buttonHeight = 0.07f;
        float winW = static_cast<float>(extent.x);
        float winH = static_cast<float>(extent.y);
        self.endTurnButton.w = winW * buttonWidth;
        self.endTurnButton.h = winH * buttonHeight;
        self.endTurnButton.x = (winW - self.endTurnButton.w) * 0.94f;
        self.endTurnButton.y = winH * 0.025f;

        return self;
    }

    void RenderHudSystem::deinit() noexcept {
        rectShader.deinit();
    }

    void RenderHudSystem::step(float /*dt*/) noexcept {

        renderHud();
        RenderTextSystem* textSystem = registry->getSystem<RenderTextSystem>();
        if (textSystem) {
            //     TODO: format tutorial view
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
                glm::vec2 textPos = {
                    pos.x + (rectBoxSize.x - textSize.x) / 2.0f,
                    pos.y + (rectBoxSize.y + textSize.y) / 2.0f - 15.0f
                };
                // Tutorial-Text
                textSystem->renderText(step->text, textPos, scale, { 1.f, 1.f, 1.f });
            }

            // TODO: update for multiple player if we do multiplayer
            Player& player = *gameState->getPlayer(0);
            std::map<types::TileType, int> resources = player.getResources();
            textSystem->renderText(
                "Wood: " + std::to_string(resources[types::TileType::FOREST]) +
                "; Stone: " + std::to_string(resources[types::TileType::MOUNTAIN]) +
                "; Grain: " + std::to_string(resources[types::TileType::FIELD]) +
                "; Round: " + std::to_string(gameState->getRoundNumber()),
                { 20.0f, 27.0f },
                0.5f,
                { 1.0f, 1.0f, 1.0f }
            );

            // End Turn Button
            renderRectBox({ endTurnButton.x , endTurnButton.y }, { endTurnButton.w, endTurnButton.h }, {0.0f, 0.0f, 1.0f});
            textSystem->renderText("End Turn", { endTurnButton.x +10 , endTurnButton.y +10 }, 0.5f, {1.0f, 1.0f, 1.0f});
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
        glm::vec2 size = { 580.f, 50.f };       // Size of the HUD

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

    bool RenderHudSystem::onMouseButton(glm::vec2 mouse, int button, int action) noexcept
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT &&
            action == GLFW_PRESS &&
            isMouseOverEndTurn(mouse))
        {
            //gameState->endTurn(); TODO: use when endTurn exists.
            gameState->setRoundNumber(gameState->getRoundNumber() + 1);
            gameState->setTurnCount(gameState->getTurnCount() + 1);
            return true;
        }
        return false;
    }


}
