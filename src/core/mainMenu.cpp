#include "mainMenu.h"
#include <iostream>

namespace df {

    void MainMenu::init(Window* windowParam) noexcept
    {
        this->window = windowParam;

        menuShader = Shader::init(assets::Shader::menu).value();

        // load textures
        titleTexture = Texture::init(assets::Texture::MENU_TITLE);
        startBtnTexture = Texture::init(assets::Texture::MENU_START);
        exitBtnTexture = Texture::init(assets::Texture::MENU_EXIT);
        backgroundTexture = Texture::init(assets::Texture::MENU_BACKGROUND);

        // create quad VAO/VBO/EBO
        initQuadBuffers();

        calcLayout();
    }

    void MainMenu::deinit() noexcept {
        menuShader.deinit();

        if (quad_vao) glDeleteVertexArrays(1, &quad_vao);
        if (quad_vbo) glDeleteBuffers(1, &quad_vbo);
        if (quad_ebo) glDeleteBuffers(1, &quad_ebo);

        quad_vao = quad_vbo = quad_ebo = 0;
    }

    void MainMenu::update(float /* delta */) noexcept {
        // hover detection (update hovered flags)
        glm::dvec2 cursor = window->getCursorPosition();
        // window returns screen coords with origin top-left
        float mouseX = static_cast<float>(cursor.x);
        float mouseY = static_cast<float>(cursor.y);

        // convert to bottom-left origin, because the layout uses bottom-left
        glm::uvec2 extent = window->getWindowExtent();
        mouseY = static_cast<float>(extent.y) - mouseY;

        startButton.hovered = isCursorOnButton(mouseX, mouseY, startButton);
        exitButton.hovered = isCursorOnButton(mouseX, mouseY, exitButton);

    }

    void MainMenu::calcLayout() noexcept {
        // size of each component, all relative to window size
        float titleWidth = 0.7f;
        float titleHeight = 0.25f;
        float buttonWidth = 0.40f;
        float buttonHeight = 0.15f;

        // offset of components in pixels
        float titleOffset = 40.0f;      // how much space is between the title and the top of the window
        float topButtonOffset = 0.45f;  // y position of the top-most button, relative to the window size
        float buttonDistance = 20.0f;   // distance between two buttons


        glm::uvec2 extent = window->getWindowExtent();
        float winW = static_cast<float>(extent.x);
        float winH = static_cast<float>(extent.y);

        // title layout (top-center)
        titleSize = glm::vec2(winW * titleWidth, winH * titleHeight);
        titlePos = glm::vec2((winW - titleSize.x) * 0.5f, winH - titleSize.y - titleOffset);

        // start button (center)
        startButton.w = winW * buttonWidth;
        startButton.h = winH * buttonHeight;
        startButton.x = (winW - startButton.w) * 0.5f;
        startButton.y = winH * topButtonOffset;

        // exit button (below start)
        exitButton.w = winW * buttonWidth;
        exitButton.h = winH * buttonHeight;
        exitButton.x = (winW - exitButton.w) * 0.5f;
        exitButton.y = startButton.y - startButton.h - buttonDistance;
    }

    void MainMenu::render() noexcept {
        if (!window) return;
        
        // determines how much more saturated an element gets, when hovered over
        float hoverIntensity = 1.25f;

        glm::uvec2 extent = window->getWindowExtent();
        float winW = static_cast<float>(extent.x);
        float winH = static_cast<float>(extent.y);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        // full-screen orthographic projection in screen pixels, origin bottom-left
        glm::mat4 projection = glm::ortho(0.0f, winW, 0.0f, winH, -1.0f, 1.0f);
        glm::mat4 view = glm::identity<glm::mat4>();

        glBindVertexArray(quad_vao);

        // helper function for drawing each element
        auto drawSprite = [&](Texture& tex, glm::vec2 pos, glm::vec2 size, glm::vec3 color) {

            glm::mat4 model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(pos, 0.0f));
            model = glm::scale(model, glm::vec3(size, 1.0f));

            tex.bind(0);
            menuShader.use()
                .setMat4("model", model)
                .setMat4("view", view)
                .setMat4("projection", projection)
                .setSampler("sprite", 0)
                .setVec3("fcolor", color);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            };

        // draw all menu elements
        drawSprite(backgroundTexture, glm::vec2(0, 0), glm::vec2(winW, winH), glm::vec3(1.0f));
        drawSprite(titleTexture, titlePos, titleSize, glm::vec3(1.0f));
        drawSprite(startBtnTexture,
            glm::vec2(startButton.x, startButton.y),
            glm::vec2(startButton.w, startButton.h),
            startButton.hovered ? glm::vec3(hoverIntensity) : glm::vec3(1.0f));
        drawSprite(exitBtnTexture,
            glm::vec2(exitButton.x, exitButton.y),
            glm::vec2(exitButton.w, exitButton.h),
            exitButton.hovered ? glm::vec3(hoverIntensity) : glm::vec3(1.0f));

        glBindVertexArray(0);
    }

    void MainMenu::onMouseButtonCallback(GLFWwindow* /*windowParam*/, int button, int action, int /* mods */) noexcept {
        // only react to LMB press
        if (button != GLFW_MOUSE_BUTTON_LEFT) return;
        if (action != GLFW_PRESS) return;

        glm::dvec2 cursor = window->getCursorPosition();
        float mouseX = static_cast<float>(cursor.x);
        float mouseY = static_cast<float>(cursor.y);

        glm::uvec2 extent = window->getWindowExtent();
        mouseY = static_cast<float>(extent.y) - mouseY;

        if (isCursorOnButton(mouseX, mouseY, startButton)) {
            fmt::println("Entering configuration");
            onStart();
        }
        else if (isCursorOnButton(mouseX, mouseY, exitButton)) {
            fmt::println("Game exited");
            onExit();
        }
    }

    void MainMenu::onKeyCallback(GLFWwindow* /*windowParam*/, int key, int /* scancode */, int action, int /* mods */) noexcept {
        // some testing for key-inputs, not fix yet
        if (action != GLFW_PRESS) return;
        if (key == GLFW_KEY_ENTER) {
            onStart();
        }
        else if (key == GLFW_KEY_ESCAPE) {
            onExit();
        }
    }

    void MainMenu::onResizeCallback(GLFWwindow*, int width, int height) noexcept {
        // Update viewport
        glViewport(0, 0, width, height);

        calcLayout();
    }


    bool MainMenu::isCursorOnButton(float mouseX, float mouseY, const Button& b) const noexcept {
        return (mouseX >= b.x && mouseX <= b.x + b.w && mouseY >= b.y && mouseY <= b.y + b.h);
    }

    void MainMenu::initQuadBuffers() noexcept {
        float quadVertices[] = {
            // positions (x,y), texture-coords (u,v)
            0.0f, 0.0f,   0.0f, 0.0f,
            1.0f, 0.0f,   1.0f, 0.0f,
            1.0f, 1.0f,   1.0f, 1.0f,
            0.0f, 1.0f,   0.0f, 1.0f
        };
        // build square from two triangles
        constexpr GLuint quadIndices[] = { 0, 1, 2, 2, 3, 0 };

        glGenVertexArrays(1, &quad_vao);
        glBindVertexArray(quad_vao);

        glGenBuffers(1, &quad_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glGenBuffers(1, &quad_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

        // layout: location 0 = positions (x,y), location 1 = texture-coords (u,v)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);
    }
} // namespace df
