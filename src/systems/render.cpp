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


	/**
	 * Initializes a mesh of one tile.
	 */
	std::vector<float> RenderSystem::createTileMesh(const float tileScale = 1.0f) noexcept {
		// Appends the hexagons corners counter-clockwise to the vertices array.
		// The center of the hexagon is at the origin.
		// It is rotated by 30 degrees in order to have a corner at the top,
		// as the tile textures already created have also the corner at the top.
		std::vector<glm::vec2> vertices;
		for (int vertex = 0; vertex < 6; vertex++) {
			const float angle = M_PI / 180.0f * (60.0f * static_cast<float>(vertex) - 30.0f);
			float x = tileScale * std::cos(angle);
			float y = tileScale * std::sin(angle);
			vertices.emplace_back(x, y);
		}

		// This is a triangulation I've come up with on my ipad.
		// The triangles are counter-clockwise as the vertices above
		// are counter-clockwise around the origin.

		std::vector<float> triangles;

		// Big center triangle
		for (int i = 0; i < 6; i += 2) {
			triangles.push_back(vertices[i].x);
			triangles.push_back(vertices[i].y);
		}

		// Three side triangles
		for (int i = 0; i < 3; i++) {
			triangles.push_back(vertices[2 * i + 0].x);
			triangles.push_back(vertices[2 * i + 0].y);
			triangles.push_back(vertices[2 * i + 1].x);
			triangles.push_back(vertices[2 * i + 1].y);
			triangles.push_back(vertices[2 * i + 2].x);
			triangles.push_back(vertices[2 * i + 2].y);
		}

		return triangles;
	}

	/**
	 * Creates a vector of the tile-specific data for the whole map arranged in a "rectangle".
	 * The tile specific data is the position and tile type, yet.
	 */
	std::vector<RenderSystem::TileInstance> RenderSystem::createTileInstances(const int columns = 10.0f, const int rows = 10.0f, const float tileScale = 1.0f) noexcept {
		std::vector<TileInstance> instances;
		for (int column = 0; column < columns; column++) {
			for (int row = 0; row < rows; row++) {
				glm::vec2 position;
				position.x = tileScale * sqrt(3.0f) * (column + 0.5f * (row & 1));
				position.y = tileScale * row * 1.5f;
				const int type = (column + row) % static_cast<int>(df::types::TileType::COUNT);

				instances.push_back({position, type});
			}
		}
		return instances;
	}

	/**
	 * For testing purposes. At first, we want to see differently colored hexagons.
	 * After that, we can implement textured hexagons.
	 * Yet, it is also undecided in the group what resolution and size the hexagons have.
	 */
	glm::vec3 RenderSystem::getTileColor(const types::TileType type) noexcept {
		switch (type) {
			case types::TileType::WATER: return {0.0f, 0.0f, 1.0f};
			case types::TileType::FOREST: return {0.0f, 0.5f, 0.0f};
			case types::TileType::GRASS: return {0.0f, 1.0f, 0.0f};
			case types::TileType::MOUNTAIN: return {0.75f, 0.75f, 0.75f};
			case types::TileType::FIELD: return {0.5f, 1.0f, 0.0f};
			case types::TileType::CLAY: return {0.5f, 0.5f, 0.5f};
			case types::TileType::ICE: return {0.75f, 0.75f, 1.0f};
			default: return {0.0f, 0.0f, 0.0f};
		}
	}

}
