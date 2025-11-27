#include "render.h"
#include <iostream>
#include "hero.h"
#include "common.h"


namespace df {

    RenderSystem RenderSystem::init(Window* window, Registry* registry) noexcept {

        RenderSystem self;

        self.window = window;
        self.registry = registry;

        self.m_viewport.m_origin = glm::uvec2(0);
        self.m_viewport.m_size = self.window->getWindowExtent();

        // load shaders
        self.spriteShader = Shader::init(assets::Shader::sprite).value();
        self.windShader = Shader::init(assets::Shader::wind).value();
        self.heroShader = Shader::init(assets::Shader::hero).value();

        glm::uvec2 extent = self.window->getWindowExtent();
        self.intermediateFramebuffer = Framebuffer::init({ static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), 1, true });

        float quadVertices[] = {
            // positions    // texcoords
            0.0f, 0.0f,     0.0f, 0.0f,
            1.0f, 0.0f,     1.0f, 0.0f,
            1.0f, 1.0f,     1.0f, 1.0f,
            0.0f, 1.0f,     0.0f, 1.0f
        };
        constexpr GLuint quadIndices[] = { 0, 1, 2, 2, 3, 0 };

        glGenVertexArrays(1, &self.m_quad_vao);
        glBindVertexArray(self.m_quad_vao);

        GLuint quadVBO;
        glGenBuffers(1, &quadVBO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        GLuint quadEBO;
        glGenBuffers(1, &quadEBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

        // Vertexattribs: pos (vec2), texcoord (vec2)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);

        // the texture buffer should be moved to another class, not in render::init but we leave it here for the
        // first milestone and change it later on when we have the logic to change between states (idling, swiming etc.)
        // maybe own class textureManager 
        std::string basePath = getBasePath();
        std::vector<std::string> heroAttackTexturePaths = {
            basePath + "/assets/textures/hero/attack/attack_0.png",
            basePath + "/assets/textures/hero/attack/attack_1.png",
            //basePath + "/assets/textures/hero/attack/attack_2.png", weird animation
        };

        for (const auto& path : heroAttackTexturePaths) {
            Texture tex = Texture::init(path.c_str());
            self.heroAttackTextures.push_back(std::move(tex));
        }

        std::vector<std::string> heroSwimTexturePaths = {
            basePath + "/assets/textures/hero/swim/swim_0.png",
            basePath + "/assets/textures/hero/swim/swim_1.png",
            basePath + "/assets/textures/hero/swim/swim_2.png",
            basePath + "/assets/textures/hero/swim/swim_3.png",
            basePath + "/assets/textures/hero/swim/swim_4.png",
            basePath + "/assets/textures/hero/swim/swim_5.png",

        };

        for (const auto& path : heroSwimTexturePaths) {
            Texture tex = Texture::init(path.c_str());
            self.heroSwimTextures.push_back(std::move(tex));
        }
       
        std::vector<std::string> heroIdleTexturePaths = {
            basePath + "/assets/textures/hero/idle/idle_0.png",
            basePath + "/assets/textures/hero/idle/idle_1.png",
            basePath + "/assets/textures/hero/idle/idle_2.png",
        };

        for (const auto& path : heroIdleTexturePaths) {
            Texture tex = Texture::init(path.c_str());
            self.heroIdleTextures.push_back(std::move(tex));
        }

        std::vector<std::string> heroJumpTexturesPath = {
            basePath + "/assets/textures/hero/jump/jump_0.png",
            basePath + "/assets/textures/hero/jump/jump_1.png",
            basePath + "/assets/textures/hero/jump/jump_2.png",
            basePath + "/assets/textures/hero/jump/jump_3.png",
            basePath + "/assets/textures/hero/jump/jump_4.png",
            basePath + "/assets/textures/hero/jump/jump_5.png",
        };

        for (const auto& path : heroJumpTexturesPath) {
            Texture tex = Texture::init(path.c_str());
            self.heroJumpTextures.push_back(std::move(tex));
        }

        // Hero entity
        // shouldnt be initialized here, but we currently dont use world.cpp so i leave it here for 
        // milestone 2 and move it to another class when our project is more strucuted
        Entity hero;
        registry->positions.emplace(hero, glm::vec2(0.5f, 0.5f));
        registry->scales.emplace(hero, glm::vec2(0.1f, 0.1f));
        registry->collisionRadius.emplace(hero, 0.5f);

        std::vector<int> animationOrder = { 0,1,2,1 }; // Sequenz 0-1-2-1 for idle
        Animation anim(animationOrder, 0.65f, true);
        registry->animations.emplace(hero, AnimationComponent{ anim });

        

        return self;
    }


    void RenderSystem::deinit() noexcept {
        spriteShader.deinit();
        windShader.deinit();
        heroShader.deinit();
    }


    Texture& RenderSystem::getCurrentTexture(AnimationComponent& animComp, int frameIndex) {
        switch (animComp.currentType) {
        case Hero::AnimationType::Idle:
            return heroIdleTextures[frameIndex];
        case Hero::AnimationType::Jump:
            return heroJumpTextures[frameIndex];
        case Hero::AnimationType::Swim:
            return heroSwimTextures[frameIndex];
        case Hero::AnimationType::Attack:
            return heroAttackTextures[frameIndex];
        default:
            return heroIdleTextures[frameIndex];
        }
    }   

    const std::vector<int>& getHeroAnimationSequence(Hero::AnimationType type) {
        switch (type) {
        case Hero::AnimationType::Idle:   return Hero::HeroAnimations::idle;
        case Hero::AnimationType::Swim:   return Hero::HeroAnimations::swim;
        case Hero::AnimationType::Jump:   return Hero::HeroAnimations::jump;
        case Hero::AnimationType::Attack: return Hero::HeroAnimations::attack;
        default:                          return Hero::HeroAnimations::idle;
        }
    }

    // sequence dereferenzen mit *sequence

    

    void RenderSystem::step(float deltaTime) noexcept {
        glm::uvec2 extent = window->getWindowExtent();
        glViewport(0, 0, extent.x, extent.y);
        glClearColor(0.674f, 0.847f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        glm::mat4 projection = glm::ortho(0.f, static_cast<float>(extent.x), 0.f, static_cast<float>(extent.y), 0.1f, 10.f);
        
        glBindVertexArray(m_quad_vao);


        for (Entity e : registry->animations.entities) {
            auto& animComp = registry->animations.get(e);
            animComp.anim.step(deltaTime);
            // not working correctly yet, needs fix
            // idea was to get the animation type like "Idle", "Swim" etc from animations.get(e) 
            // and then we chose the correct textureBuffer and the correct animationSequence and render
            // the animation, but currently the animationSequence is fixed and cant be changed in this loop
            // needs change, but it works to display hero idle animation for milestone 2
            int texIndex = animComp.anim.getCurrentFrameTextureIndex(); 
            Texture& tex = getCurrentTexture(animComp, texIndex);
            tex.bind(0);
            std::vector<int> animationOrder = getHeroAnimationSequence(animComp.currentType);


            glm::vec2 pos = registry->positions.get(e);
            glm::vec2 scale = registry->scales.get(e);
            glm::vec2 pixelPos = glm::vec2(pos.x * extent.x, pos.y * extent.y);
            glm::vec2 pixelScale = glm::vec2(scale.x * extent.x, scale.y * extent.y);



            glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(pixelPos, 0));
            model = glm::scale(model, glm::vec3(pixelScale, 1));

            Shader& shader = heroShader;
            shader.use()
                .setMat4("model", model)
                .setMat4("view", view)
                .setMat4("projection", projection)
                .setVec3("fcolor", glm::vec3(1.0f));

        }

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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