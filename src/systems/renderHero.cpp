#include "renderHero.h"
#include "hero.h"
#include "../core/tile.h"
#include "utils/textureArray.h"
#include "common.h"

namespace df {

    RenderHeroSystem RenderHeroSystem::init(Window* window, Registry* registry, std::shared_ptr<GameState> gameState) noexcept {

        RenderHeroSystem self;

        self.window = window;
        self.registry = registry;
        self.gameState = gameState;

        self.viewport.origin = glm::uvec2(0);
        self.viewport.size = self.window->getWindowExtent();


        // load resources for rendering
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


        self.heroAttackTextures = {
            Texture::init(assets::getAssetPath(assets::Texture::HERO_ATTACK_0).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_ATTACK_1).c_str())

        };

        self.heroSwimTextures = {
            Texture::init(assets::getAssetPath(assets::Texture::HERO_SWIM_0).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_SWIM_1).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_SWIM_2).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_SWIM_3).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_SWIM_4).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_SWIM_5).c_str()),
        };

        self.heroIdleTextures = {
            Texture::init(assets::getAssetPath(assets::Texture::HERO_IDLE_0).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_IDLE_1).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_IDLE_2).c_str()),
        };

        self.heroJumpTextures = {
            Texture::init(assets::getAssetPath(assets::Texture::HERO_JUMP_0).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_JUMP_1).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_JUMP_2).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_JUMP_3).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_JUMP_4).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_JUMP_5).c_str()),
        };

        self.heroRunTextures = {
            Texture::init(assets::getAssetPath(assets::Texture::HERO_RUN_0).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_RUN_1).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_RUN_2).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_RUN_3).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_RUN_4).c_str()),
            Texture::init(assets::getAssetPath(assets::Texture::HERO_RUN_5).c_str()),
        };

        
        Entity hero;
      
        registry->positions.emplace(hero, glm::vec2(1.0f, 1.0f));
        registry->scales.emplace(hero, glm::vec2(1.0f, 1.0f));
        registry->collisionRadius.emplace(hero, 0.5f);


        std::vector animationOrder = { 0,1 }; 
        Animation anim(animationOrder, 0.65f, true);
        registry->animations.emplace(hero, AnimationComponent{ anim });

        //constexpr std::array<GLuint, 6> indices = { 0, 1, 2, 2, 3, 0 };

        return self;
    }



    void RenderHeroSystem::updateDimensionsFromMap() noexcept {
        if (!gameState) return;

        const Graph& map = gameState->getMap();
        unsigned mapColumns = map.getMapWidth();
        unsigned tileCount = map.getTileCount();

        this->columns = mapColumns;
        this->rows = tileCount / mapColumns;
        fmt::println("INIT ERFOLGREICH");
    }

    void RenderHeroSystem::deinit() noexcept {
        heroShader.deinit();
    }


    Texture& RenderHeroSystem::getCurrentTexture(AnimationComponent& animComp, int frameIndex) {
        switch (animComp.currentType) {
        case Hero::AnimationType::Idle:   return heroIdleTextures[frameIndex];
        case Hero::AnimationType::Jump:   return heroJumpTextures[frameIndex];
        case Hero::AnimationType::Swim:   return heroSwimTextures[frameIndex];
        case Hero::AnimationType::Run:    return heroRunTextures[frameIndex];
        case Hero::AnimationType::Attack: return heroAttackTextures[frameIndex];
        default:                          return heroIdleTextures[frameIndex];
        }
    }

    const std::vector<int>& getHeroAnimationSequence(Hero::AnimationType type) {
        switch (type) {
        case Hero::AnimationType::Idle:   return Hero::HeroAnimations::idle;
        case Hero::AnimationType::Swim:   return Hero::HeroAnimations::swim;
        case Hero::AnimationType::Jump:   return Hero::HeroAnimations::jump;
        case Hero::AnimationType::Run:    return Hero::HeroAnimations::run;
        case Hero::AnimationType::Attack: return Hero::HeroAnimations::attack;
        default:                          return Hero::HeroAnimations::idle;
        }
    }

    void RenderHeroSystem::step(float deltaTime) noexcept {


        glm::uvec2 extent = window->getWindowExtent();
        Camera& cam = registry->cameras.get(registry->getCamera());
        glViewport(0, 0, extent.x, extent.y);
        //glClearColor(0.674f, 0.847f, 1.0f, 1.0f);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        glm::vec2 worldDims = calculateWorldDimensions(this->columns, this->rows);
        glm::mat4 projection = glm::ortho(
            cam.position.x, cam.position.x + worldDims.x / cam.zoom,
            cam.position.y, cam.position.y + worldDims.y / cam.zoom,
            -1.f, 1.f
        );
        glm::mat4 view = glm::mat4(1.f);

        glBindVertexArray(m_quad_vao);

        for (Entity e : registry->animations.entities) {
            auto& animComp = registry->animations.get(e);
            std::vector<int> animationOrder = getHeroAnimationSequence(animComp.currentType);
            animComp.anim.setFrames(animationOrder);
            animComp.anim.step(deltaTime);

            int texIndex = animComp.anim.getCurrentFrameTextureIndex();
            Texture& tex = getCurrentTexture(animComp, texIndex);
            tex.bind(0);

            //glm::vec2 screenPos = glm::vec2(0.5f, 0.5f) * glm::vec2(window->getWindowExtent());

            glm::vec2 heroPos = registry->positions.get(e);
            glm::vec2 worldScale = registry->scales.get(e);

            glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(heroPos - cam.position, 0.f));
            model = glm::scale(model, glm::vec3(worldScale, 1.f));

            Shader& shader = heroShader;
            shader.use()
                .setMat4("model", model)
                .setMat4("view", view)
                .setMat4("projection", projection)
                .setVec3("fcolor", glm::vec3(1.0f));
        }

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    void RenderHeroSystem::reset() noexcept {}

}
