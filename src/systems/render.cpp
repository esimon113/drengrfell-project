#include "render.h"
#include <iostream>
#include "common.h"


namespace df {

    RenderSystem RenderSystem::init(Window* window, Registry* registry) noexcept {
        RenderSystem self;

        self.window = window;
        self.registry = registry;

        self.viewport.origin = glm::uvec2(0);
        self.viewport.size = self.window->getWindowExtent();

		const glm::uvec2 extent = self.window->getWindowExtent();
		self.intermediateFramebuffer = Framebuffer::init({ static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), 1, true });

		// This shall be the place for initialization of ALL render systems. NOT anywhere else!
    	self.renderHeroSystem = RenderHeroSystem::init(window, registry);
    	self.renderTilesSystem = RenderTilesSystem::init(window, registry);
		self.renderBuildingsSystem = RenderBuildingsSystem::init(window, registry);

		return self;
	}


	void RenderSystem::deinit() noexcept {
    	this->renderTilesSystem.deinit();
    	this->renderBuildingsSystem.deinit();
    	this->renderHeroSystem.deinit();
	}

    void RenderSystem::step(const float dt) noexcept {
    	this->renderTilesSystem.step(dt);
    	this->renderBuildingsSystem.step(dt);
    	this->renderHeroSystem.step(dt);
    }


    void RenderSystem::reset() noexcept {
    	this->renderTilesSystem.reset();
    	this->renderBuildingsSystem.reset();
    	this->renderHeroSystem.reset();
    }


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
        this->viewport.origin = origin;
        this->viewport.size = size;

		// reinitialize off-screen framebuffer
		intermediateFramebuffer.deinit();
		intermediateFramebuffer = Framebuffer::init({ (GLsizei)size.x, (GLsizei)size.y, 1, true });
	}
}
