#include "configMenu.h"
#include <iostream>

namespace df {

    void ConfigMenu::init(Window* windowParam, Registry* registryParam) noexcept
    {
        this->window = windowParam;
        this->registry = registryParam;

        menuShader = Shader::init(assets::Shader::menu).value();

        // load textures
        aiBtnTexture = Texture::init(assets::MenuTexture::CONFIG_AI);
        backgroundTexture = Texture::init(assets::MenuTexture::CONFIG_BACKGROUND);
        easyBtnTexture = Texture::init(assets::MenuTexture::CONFIG_EASY);
        hardBtnTexture = Texture::init(assets::MenuTexture::CONFIG_HARD);
        heightBtnTexture = Texture::init(assets::MenuTexture::CONFIG_HEIGHT);
        insularBtnTexture = Texture::init(assets::MenuTexture::CONFIG_INSULAR);
        mediumBtnTexture = Texture::init(assets::MenuTexture::CONFIG_MEDIUM);
        multiplayerBtnTexture = Texture::init(assets::MenuTexture::CONFIG_MULTIPLAYER);
        perlinBtnTexture = Texture::init(assets::MenuTexture::CONFIG_PERLIN);
        seedBtnTexture = Texture::init(assets::MenuTexture::CONFIG_SEED);
        startBtnTexture = Texture::init(assets::MenuTexture::CONFIG_START);
        titleTexture = Texture::init(assets::MenuTexture::CONFIG_TITLE);
        widthBtnTexture = Texture::init(assets::MenuTexture::CONFIG_WIDTH);

        // create quad VAO/VBO/EBO
        initQuadBuffers();

        calcLayout();
    }

    void ConfigMenu::deinit() noexcept {
        menuShader.deinit();

        if (quad_vao) glDeleteVertexArrays(1, &quad_vao);
        if (quad_vbo) glDeleteBuffers(1, &quad_vbo);
        if (quad_ebo) glDeleteBuffers(1, &quad_ebo);

        quad_vao = quad_vbo = quad_ebo = 0;
    }


    void ConfigMenu::update(float  delta ) noexcept {
        if (warningTimer > 0.0f)
            warningTimer -= delta;
        capInputValue();
        // hover detection (update hovered flags)
        glm::dvec2 cursor = window->getCursorPosition();
        // window returns screen coords with origin top-left
        float mouseX = static_cast<float>(cursor.x);
        float mouseY = static_cast<float>(cursor.y);

        // convert to bottom-left origin, because the layout uses bottom-left
        glm::uvec2 extent = window->getWindowExtent();
        mouseY = static_cast<float>(extent.y) - mouseY;

        startButton.hovered = isCursorOnButton(mouseX, mouseY, startButton);
        insularButton.hovered = isCursorOnButton(mouseX, mouseY, insularButton);
        perlinButton.hovered = isCursorOnButton(mouseX, mouseY, perlinButton);
        seedButton.hovered = isCursorOnButton(mouseX, mouseY, seedButton);
        widthButton.hovered = isCursorOnButton(mouseX, mouseY, widthButton);
        /*
        * heightButton is used when map can be non quadratic
        heightButton.hovered = isCursorOnButton(mouseX, mouseY, heightButton);
        */

    }

    void ConfigMenu::calcLayout() noexcept {
        glm::uvec2 extent = window->getWindowExtent();
        float winW = static_cast<float>(extent.x);
        float winH = static_cast<float>(extent.y);

        // size of each component, all relative to window size
        float relTitleWidth = 0.7f;
        float relTitleHeight = 0.25f;
        float relButtonWidth = 0.25f;
        float relButtonHeight = 0.10f;

        // offset of components, all relative to window size
        float relTitleOffsetY = 0.1f;      // how much space is between the title and the top of the window
        float relTopButtonOffsetY = 0.45f;  // y position of the top-most button, relative to the window size
        float relButtonDistanceX = 0.02f;   // distance between two buttons on the x-axis
        float relButtonDistanceY = 0.02f;   // distance between two buttons on the y-axis

        // all of the above values, converted to pixel values
        float absTitleWidth = relTitleWidth * winW;
        float absTitleHeight = relTitleHeight * winH;
        float absButtonWidth = relButtonWidth * winW;
        float absButtonHeight = relButtonHeight * winH;

        float absTitleOffsetY = relTitleOffsetY * winH;
        float absTopButtonOffsetY = relTopButtonOffsetY * winH;
        float absButtonDistanceX = relButtonDistanceX * winW;
        float absButtonDistanceY = relButtonDistanceY * winH;

        // title layout (top-center)
        titleSize = glm::vec2(absTitleWidth, absTitleHeight);
        titlePos = glm::vec2((winW - titleSize.x) * 0.5f, winH - titleSize.y - absTitleOffsetY);

        float xMiddle = (winW - absButtonWidth) * 0.5f;
        float xLeft = (winW - absButtonDistanceX) * 0.5f - absButtonWidth;
        float xRight = (winW + absButtonDistanceX) * 0.5f;

        seedButton.x = xMiddle;
        seedButton.y = absTopButtonOffsetY;
        seedButton.w = absButtonWidth;
        seedButton.h = absButtonHeight;

        insularButton.x = xLeft;
        insularButton.y = absTopButtonOffsetY - (absButtonDistanceY + absButtonHeight);
        insularButton.w = absButtonWidth;
        insularButton.h = absButtonHeight;

        perlinButton.x = xRight;
        perlinButton.y = absTopButtonOffsetY - (absButtonDistanceY + absButtonHeight);
        perlinButton.w = absButtonWidth;
        perlinButton.h = absButtonHeight;

        // TEMPORARY START while we only support quadratic maps
        widthButton.x = xMiddle;
        // TEMPORARY END
        //widthButton.x = xLeft;
        widthButton.y = absTopButtonOffsetY - 2 * (absButtonDistanceY + absButtonHeight);
        widthButton.w = absButtonWidth;
        widthButton.h = absButtonHeight;

        /*
        * Keeping the map quadratic for this milestone
        heightButton.x = xRight;
        heightButton.y = absTopButtonOffsetY - 2 * (absButtonDistanceY + absButtonHeight);
        heightButton.w = absButtonWidth;
        heightButton.h = absButtonHeight;
        */

        

        startButton.x = xMiddle;
        startButton.y = absTopButtonOffsetY - 3 * (absButtonDistanceY + absButtonHeight);
        startButton.w = absButtonWidth;
        startButton.h = absButtonHeight;

        warningPos = { winW * 0.1f, winH * 0.95f };
        infoPos = { titlePos - glm::vec2{0.0f, winH * 0.03f} };
    }

    void ConfigMenu::render() noexcept {
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
        drawSprite(insularBtnTexture,
            glm::vec2(insularButton.x, insularButton.y),
            glm::vec2(insularButton.w, insularButton.h),
            insularButton.hovered ? glm::vec3(hoverIntensity) : glm::vec3(1.0f));
        drawSprite(perlinBtnTexture,
            glm::vec2(perlinButton.x, perlinButton.y),
            glm::vec2(perlinButton.w, perlinButton.h),
            perlinButton.hovered ? glm::vec3(hoverIntensity) : glm::vec3(1.0f));
        drawSprite(seedBtnTexture,
            glm::vec2(seedButton.x, seedButton.y),
            glm::vec2(seedButton.w, seedButton.h),
            seedButton.hovered ? glm::vec3(hoverIntensity) : glm::vec3(1.0f));
        drawSprite(widthBtnTexture,
            glm::vec2(widthButton.x, widthButton.y),
            glm::vec2(widthButton.w, widthButton.h),
            widthButton.hovered ? glm::vec3(hoverIntensity) : glm::vec3(1.0f));
        /*
        * heightButton is used when map can be non quadratic
        drawSprite(heightBtnTexture,
            glm::vec2(heightButton.x, heightButton.y),
            glm::vec2(heightButton.w, heightButton.h),
            heightButton.hovered ? glm::vec3(hoverIntensity) : glm::vec3(1.0f));
        */

        glBindVertexArray(0);

        RenderTextSystem* textSystem = registry->getSystem<RenderTextSystem>();
        if (textSystem) {
            // renders the user keyboard-input
            textSystem->renderText(
                inputString,
                { 20.0f, 27.0f },
                0.5f,
                { 1.0f, 1.0f, 1.0f }
            );

            if(seedButton.hovered) {
                textSystem->renderText(
                    "Click this button to start the seed input.\n"
                    "You can type a number between 1 and 100 \n"
                    "and confirm with enter.",
                    infoPos,
                    0.4f,
                    { 1.0f, 0.0f, 0.0f }
                );
            }
            if (insularButton.hovered) {
                textSystem->renderText(
                    "Click this button to select the generation mode insular.",
                    infoPos,
                    0.4f,
                    { 1.0f, 0.0f, 0.0f }
                );
            }
            if (perlinButton.hovered) {
                textSystem->renderText(
                    "Click this button to select the generation mode perlin.",
                    infoPos,
                    0.4f,
                    { 1.0f, 0.0f, 0.0f }
                );
            }
            if (widthButton.hovered) {
                textSystem->renderText(
                    "Click this button to start the map-size input.\n"
                    "You can type a number between 1 and 100 \n"
                    "and confirm with enter.",
                    infoPos,
                    0.4f,
                    { 1.0f, 0.0f, 0.0f }
                );
            }
            if (startButton.hovered) {
                textSystem->renderText(
                    "Click this button to start the game.\n"
                    "Not selecting parameters will \n"
                    "use the ones from the previous map.",
                    infoPos,
                    0.4f,
                    { 1.0f, 0.0f, 0.0f }
                );
            }
        }
        if (warningTimer > 0.0f) {
            renderWarning();
        }
    }

    void ConfigMenu::onMouseButtonCallback(GLFWwindow* /*windowParam*/, int button, int action, int /* mods */) noexcept {
        // only react to LMB press
        if (button != GLFW_MOUSE_BUTTON_LEFT) return;
        if (action != GLFW_PRESS) return;

        glm::dvec2 cursor = window->getCursorPosition();
        float mouseX = static_cast<float>(cursor.x);
        float mouseY = static_cast<float>(cursor.y);

        glm::uvec2 extent = window->getWindowExtent();
        mouseY = static_cast<float>(extent.y) - mouseY;

        if (isCursorOnButton(mouseX, mouseY, startButton)) {
            fmt::println("Game started");
            onStart(
                worldSeed,
                worldWidth,
                worldHeight,
                worldGenerationMode
            );
        }
        if (isCursorOnButton(mouseX, mouseY, insularButton)) {
            fmt::println("Insular generation chosen");
            worldGenerationMode = 0;
        }
        if (isCursorOnButton(mouseX, mouseY, perlinButton)) {
            fmt::println("Perlin generation chosen");
            worldGenerationMode = 1;
        }
        if (isCursorOnButton(mouseX, mouseY, seedButton)) {
            fmt::println("Enter seed");
            activeInput = InputField::SEED;
            inputString.clear();
        }
        if (isCursorOnButton(mouseX, mouseY, widthButton)) {
            fmt::println("Enter width");
            activeInput = InputField::WIDTH;
            inputString.clear();
        }
        /*
        * heightButton is used when map can be non quadratic
        if (isCursorOnButton(mouseX, mouseY, heightButton)) {
            fmt::println("Enter height");
            activeInput = InputField::HEIGHT;
            inputString.clear();
        }
        */

    }

    void ConfigMenu::renderWarning() noexcept {
        RenderTextSystem* textSystem = registry->getSystem<RenderTextSystem>();
        if (textSystem) {
            // renders a warning - input too small
            textSystem->renderText(
                warningMessage,
                warningPos,
                0.5f,
                { 1.0f, 0.0f, 0.0f }
            );
        }
    }

    void ConfigMenu::capMapsize() noexcept {
        if (activeInput == InputField::WIDTH || activeInput == InputField::HEIGHT) {
            int value = std::stoi(inputString);
            if (value < 1) {
                warningTimer = 2.0f;
                warningMessage = "Map size too small, must be >= 1";
                inputString = "1";
            }
            if (value > 100) {
                warningTimer = 2.0f;
                warningMessage = "Map size too big, must be <= 100";
                inputString = "100";
            }
        }
    }

    void ConfigMenu::capInputValue() noexcept {
        if (inputString.length() > 10) { // INT_MAX = 2147483647 so more than 10 digits are not allowed
            warningTimer = 2.0f;
            warningMessage = "Input length too long, defaulting to INT_MAX";
            fmt::println("Input length too long, defaulting to INT_MAX");
            inputString = std::to_string(INT_MAX);
        }
        else if (inputString.length() == 10 && inputString > "2147483647") {
            warningTimer = 2.0f;
            warningMessage = "Value too large, defaulting to INT_MAX";
            fmt::println("Value too large, defaulting to INT_MAX");
            inputString = std::to_string(INT_MAX);
        }
        if (inputString.length() > 0) capMapsize();
    }

    void ConfigMenu::onKeyCallback(GLFWwindow* /*windowParam*/, int key, int /* scancode */, int action, int /* mods */) noexcept {
        
        if (activeInput != InputField::NONE && action == GLFW_PRESS) {
            // only read numbers
            if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
                // append the pressed key to the number
                inputString += static_cast<char>('0' + (key - GLFW_KEY_0));
            }
            // delete the last character on backspace
            else if (key == GLFW_KEY_BACKSPACE && !inputString.empty()) {
                inputString.pop_back();
            }
            // finish input with enter
            else if (key == GLFW_KEY_ENTER) {
                if (inputString.empty()) {
                    fmt::println("Input an empty string"); 
                    activeInput = InputField::NONE;
                    inputString.clear();
                    return;
                }
                int value = std::stoi(inputString);
                switch (activeInput) {
                case InputField::SEED:
                    worldSeed = value;
                    fmt::println("Seed: {}", value);
                    break;
                case InputField::WIDTH:
                    worldWidth = value;
                    fmt::println("width: {}", value);
                    // TEMPORARY while map is only quadratic
                    worldHeight = value;
                    fmt::println("height: {}", value);
                    break;
                case InputField::HEIGHT:
                    worldHeight = value;
                    fmt::println("height: {}", value);
                    break;
                default:
                    break;
                }
                activeInput = InputField::NONE;
                inputString.clear();
            }
            // Pressing esc cancels the input
            else if (key == GLFW_KEY_ESCAPE) {
                activeInput = InputField::NONE;
                inputString.clear();
            }
        }

    }

    void ConfigMenu::onResizeCallback(GLFWwindow*, int width, int height) noexcept {
        // Update viewport
        glViewport(0, 0, width, height);

        
        if (auto* textSystem = registry->getSystem<RenderTextSystem>()) {
            textSystem->updateViewport({ 0, 0 },{ static_cast<unsigned>(width), static_cast<unsigned>(height) });
        }
        calcLayout();
    }


    bool ConfigMenu::isCursorOnButton(float mouseX, float mouseY, const Button& b) const noexcept {
        return (mouseX >= b.x && mouseX <= b.x + b.w && mouseY >= b.y && mouseY <= b.y + b.h);
    }

    void ConfigMenu::initQuadBuffers() noexcept {
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
