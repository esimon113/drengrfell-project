#include "render.h"



namespace df {
	RenderSystem RenderSystem::init(Window* window, Registry* registry) noexcept {
		RenderSystem self;

		self.window = window;
		self.registry = registry;

		self.m_viewport.m_origin = glm::uvec2(0);
		self.m_viewport.m_size = self.window->getWindowExtent();

		// load resources for rendering
		self.spriteShader = Shader::init(assets::Shader::sprite).value();
		self.windShader = Shader::init(assets::Shader::wind).value();
		// ...

		glm::uvec2 extent = self.window->getWindowExtent();
		self.intermediateFramebuffer = Framebuffer::init({ static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), 1, true });

		glGenVertexArrays(1, &self.m_quad_vao);
		glBindVertexArray(self.m_quad_vao);
		glGenBuffers(1, &self.m_quad_ebo);

		constexpr ::std::array<GLuint, 6> indices = { 0, 1, 2, 2, 3, 0 };
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self.m_quad_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);


		return self;
	}


	void RenderSystem::deinit() noexcept {
		spriteShader.deinit();
		windShader.deinit();
	}


	void RenderSystem::step(const float) noexcept {
		// ...
	}


	void RenderSystem::reset() noexcept {}


	static constexpr float GAME_ASPECT_RATIO = 1.f/2; //< Aspect ratio of the in-game map.


	static std::pair<glm::uvec2, glm::uvec2> computeViewportConfig(const glm::uvec2 resolution) noexcept {
		float window_aspect_ratio = (float)resolution.x/resolution.y;
		if (window_aspect_ratio > GAME_ASPECT_RATIO) {
			glm::uvec2 viewport_size = { resolution.y * GAME_ASPECT_RATIO, resolution.y };
			uint32_t offset = (resolution.x - viewport_size.x) / 2;
			glm::uvec2 viewport_offset = { offset, 0 };

			return { viewport_offset, viewport_size };
		}

		glm::uvec2 viewport_size = { resolution.x, resolution.x * 1/GAME_ASPECT_RATIO };
		uint32_t offset = (resolution.y - viewport_size.y) / 2;
		glm::uvec2 viewport_offset = { 0, offset };

		return { viewport_offset, viewport_size };
	}


	void RenderSystem::onResizeCallback(GLFWwindow*, int width, int height) noexcept {
		auto [origin, size] = computeViewportConfig({width, height});

		m_viewport.m_origin = origin;
		m_viewport.m_size = size;

		// reinitialize off-screen framebuffer
		intermediateFramebuffer.deinit();
		intermediateFramebuffer = Framebuffer::init({ (GLsizei)size.x, (GLsizei)size.y, 1, true });
	}
}
