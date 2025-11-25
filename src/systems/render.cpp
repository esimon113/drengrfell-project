#include "render.h"
#include <iostream>

namespace df {

    RenderSystem RenderSystem::init(Window* window, Registry* registry) noexcept {
        std::cout << "[Debug] RenderSystem::init aufgerufen" << std::endl;
        RenderSystem self;

        self.window = window;
        self.registry = registry;

        self.m_viewport.m_origin = glm::uvec2(0);
        self.m_viewport.m_size = self.window->getWindowExtent();

        // load resources for rendering
        self.spriteShader = Shader::init(assets::Shader::sprite).value();
        self.windShader = Shader::init(assets::Shader::wind).value();

        glm::uvec2 extent = self.window->getWindowExtent();
        self.intermediateFramebuffer = Framebuffer::init({ static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), 1, true });

        glGenVertexArrays(1, &self.m_quad_vao);
        glBindVertexArray(self.m_quad_vao);
        glGenBuffers(1, &self.m_quad_ebo);

        constexpr std::array<GLuint, 6> indices = { 0, 1, 2, 2, 3, 0 };
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self.m_quad_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);

        for (int i = 0; i < 5; ++i) {
            std::string path = fmt::format("team07/assets/textures/hero/idle/idle_{}.png", i);
            Texture tex = Texture::init(path.c_str());
            if (tex == 0) {
                std::cout << "[ERROR] Texture konnte nicht geladen werden: " << path << std::endl;
            }
            self.heroIdleTextures.push_back(std::move(tex));
        }

        Entity hero = registry->createEntity();
        registry->positions.emplace(hero, glm::vec2(0.5f, 0.5f));
        registry->scales.emplace(hero, glm::vec2(0.1f, 0.1f));
        registry->collisionRadius.emplace(hero, 0.5f);

        std::vector<std::string> idleFrames = {
            "assets/textures/hero/idle/idle_0.png",
            "assets/textures/hero/idle/idle_1.png",
            "assets/textures/hero/idle/idle_2.png",
            "assets/textures/hero/idle/idle_3.png",
            "assets/textures/hero/idle/idle_4.png",
        };

        Animation idleAnim(idleFrames, 0.4f, true); // 1 picture for 0.4 seconds
        registry->animations.emplace(hero, AnimationComponent{ idleAnim });

        return self;
    }

    void RenderSystem::deinit() noexcept {
        spriteShader.deinit();
        windShader.deinit();
    }

    void RenderSystem::step(float deltaTime) noexcept {
        glm::uvec2 extent = window->getWindowExtent();
        glViewport(0, 0, extent.x, extent.y);
        glClearColor(0.674f, 0.847f, 1.0f, 1.0f); // Hintergrund hellblau
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        glm::mat4 projection = glm::ortho(0.f, static_cast<float>(extent.x), 0.f, static_cast<float>(extent.y), 0.1f, 10.f);

        std::cout << "[Debug] Anzahl Animations-Entities: " << registry->animations.entities.size() << std::endl;

        for (Entity e : registry->animations.entities) {
            auto& animComp = registry->animations.get(e);
            animComp.anim.step(deltaTime);

            int frameIndex = animComp.anim.getCurrentFrameIndex();
            frameIndex = std::clamp(frameIndex, 0, static_cast<int>(heroIdleTextures.size() - 1));
            Texture& spriteTexture = heroIdleTextures[frameIndex];

            glm::vec2 normPos = registry->positions.get(e);
            glm::vec2 normScale = registry->scales.get(e);

            glm::vec2 pixelPos = glm::vec2(normPos.x * extent.x, normPos.y * extent.y);
            glm::vec2 pixelScale = glm::vec2(normScale.x * extent.x, normScale.y * extent.y);

            glm::mat4 model = glm::identity<glm::mat4>();
            model = glm::translate(model, glm::vec3(pixelPos, 0));
            model = glm::scale(model, glm::vec3(pixelScale, 1));

            spriteShader.use()
                .setMat4("model", model)
                .setMat4("view", view)
                .setMat4("projection", projection)
                .setSampler("sprite", 0);

            spriteTexture.bind(0);
            glBindVertexArray(m_quad_vao);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        intermediateFramebuffer.unbind();

        extent = window->getWindowExtent();
        glViewport(0, 0, extent.x, extent.y);
        glClearColor(0, 0, 0, 1);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void RenderSystem::reset() noexcept {}

    static constexpr float GAME_ASPECT_RATIO = 1.f / 2;

    static std::pair<glm::uvec2, glm::uvec2> computeViewportConfig(const glm::uvec2 resolution) noexcept {
        float window_aspect_ratio = static_cast<float>(resolution.x) / resolution.y;
        if (window_aspect_ratio > GAME_ASPECT_RATIO) {
            glm::uvec2 viewport_size = { resolution.y * GAME_ASPECT_RATIO, resolution.y };
            uint32_t offset = (resolution.x - viewport_size.x) / 2;
            glm::uvec2 viewport_offset = { offset, 0 };
            return { viewport_offset, viewport_size };
        }

        glm::uvec2 viewport_size = { resolution.x, resolution.x * 1 / GAME_ASPECT_RATIO };
        uint32_t offset = (resolution.y - viewport_size.y) / 2;
        glm::uvec2 viewport_offset = { 0, offset };
        return { viewport_offset, viewport_size };
    }

    void RenderSystem::onResizeCallback(GLFWwindow*, int width, int height) noexcept {
        auto [origin, size] = computeViewportConfig({ width, height });
        m_viewport.m_origin = origin;
        m_viewport.m_size = size;

        intermediateFramebuffer.deinit();
        intermediateFramebuffer = Framebuffer::init({ static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y), 1, true });
    }

} // namespace df