#include "render.h"
#include <iostream>
#include "common.h"


namespace df {

    RenderSystem RenderSystem::init(Window* window, Registry* registry, GameState& gameState) noexcept {
        RenderSystem self;

        self.window = window;
        self.registry = registry;

        self.viewport.origin = glm::uvec2(0);
        self.viewport.size = self.window->getWindowExtent();

		const glm::uvec2 extent = self.window->getWindowExtent();
		self.intermediateFramebuffer = Framebuffer::init({ static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), 1, true });

        self.renderTilesSystem = RenderTilesSystem::init(*window, *registry, gameState);
    	self.renderHeroSystem = RenderHeroSystem::init(window, registry, gameState);
		self.renderBuildingsSystem = RenderBuildingsSystem::init(window, registry);
        self.renderHudSystem = RenderHudSystem::init(window, registry, gameState);
        self.renderTextSystem = RenderTextSystem::init(window, registry);
        self.renderSnowSystem = RenderSnowSystem::init(window,registry);

		return self;
	}


	void RenderSystem::deinit() noexcept {
    	this->renderTilesSystem.deinit();
    	this->renderBuildingsSystem.deinit();
    	this->renderHeroSystem.deinit();
    	this->renderHudSystem.deinit();
    	this->renderTextSystem.deinit();
        this->renderSnowSystem.deinit();
	}

    void RenderSystem::step(const float dt) noexcept {
    	this->renderTilesSystem.step(dt);
    	this->renderBuildingsSystem.step(dt);
    	this->renderHeroSystem.step(dt);
    	this->renderTextSystem.step(dt);
        this->renderSnowSystem.step(dt);
        this->renderHudSystem.step(dt); // always rendered last
        
    }


    void RenderSystem::reset() noexcept {
    	this->renderTilesSystem.reset();
    	this->renderBuildingsSystem.reset();
    	this->renderHeroSystem.reset();
    	this->renderHudSystem.reset();
    	this->renderTextSystem.reset();
        this->renderSnowSystem.reset();
    }


    static constexpr float GAME_ASPECT_RATIO = 1.f / 2;


    static std::pair<glm::uvec2, glm::uvec2> computeViewportConfig(const glm::uvec2 resolution) noexcept {
        // Use the full window
        // float window_aspect_ratio = static_cast<float>(resolution.x) / resolution.y;
        // if (window_aspect_ratio > GAME_ASPECT_RATIO) {
        //     glm::uvec2 viewport_size = { resolution.y * GAME_ASPECT_RATIO, resolution.y };
        //     uint32_t offset = (resolution.x - viewport_size.x) / 2;
        //     glm::uvec2 viewport_offset = { offset, 0 };
        //     return { viewport_offset, viewport_size };
        // }

        // glm::uvec2 viewport_size = { resolution.x, resolution.x * 1 / GAME_ASPECT_RATIO };
        // uint32_t offset = (resolution.y - viewport_size.y) / 2;
        // glm::uvec2 viewport_offset = { 0, offset };
        // return { viewport_offset, viewport_size };
        return { glm::uvec2(0, 0), resolution };
    }


    void RenderSystem::onResizeCallback(GLFWwindow*, int width, int height) noexcept {
        auto [origin, size] = computeViewportConfig({ width, height });
        this->viewport.origin = origin;
        this->viewport.size = size;


        this->renderBuildingsSystem.updateViewport(origin, size);
        this->renderTextSystem.updateViewport(origin, size);
        this->renderHudSystem.updateViewport(origin,size);
        
		// reinitialize off-screen framebuffer
		intermediateFramebuffer.deinit();
		intermediateFramebuffer = Framebuffer::init({ (GLsizei)size.x, (GLsizei)size.y, 1, true });
        
	}

    void RenderSystem::onKeyCallback(GLFWwindow *pwindow, int key, int scancode, int action, int mods) noexcept {
    	this->renderTilesSystem.onKeyCallback(pwindow, key, scancode, action, mods);
    }
}
