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
		self.tileShader = Shader::init(assets::Shader::tile).value();
		// ...

		glm::uvec2 extent = self.window->getWindowExtent();
		self.intermediateFramebuffer = Framebuffer::init({ static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), 1, true });

		glGenVertexArrays(1, &self.m_quad_vao);
		glBindVertexArray(self.m_quad_vao);
		glGenBuffers(1, &self.m_quad_ebo);

		constexpr ::std::array<GLuint, 6> indices = { 0, 1, 2, 2, 3, 0 };
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self.m_quad_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);

		self.initMap();

		return self;
	}


	void RenderSystem::deinit() noexcept {
		spriteShader.deinit();
		windShader.deinit();
		tileShader.deinit();
	}


	void RenderSystem::step(const float) noexcept {
		renderMap();
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
	std::vector<float> RenderSystem::createTileMesh(const float tileScale) noexcept {
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
			triangles.push_back(vertices[(2 * i + 2) % 6].x);
			triangles.push_back(vertices[(2 * i + 2) % 6].y);
		}

		return triangles;
	}

	/**
	 * Creates a vector of the tile-specific data for the whole map arranged in a "rectangle".
	 * The tile specific data is the position and tile type, yet.
	 */
	std::vector<RenderSystem::TileInstance> RenderSystem::createTileInstances(const int columns, const int rows, const float tileScale) noexcept {
		std::vector<TileInstance> instances;
		for (int column = 0; column < columns; column++) {
			for (int row = 0; row < rows; row++) {
				glm::vec2 position;
				position.x = tileScale * sqrt(3.0f) * (column + 0.5f * (row & 1));
				position.y = tileScale * row * 1.5f;
				const int type = (column + row) % static_cast<int>(df::types::TileType::COUNT);

				instances.push_back({position, type, 0});
			}
		}
		return instances;
	}

	/**
	 * Initializes the map data
	 */
	void RenderSystem::initMap() noexcept {
		this->tileMesh = createTileMesh();
		this->tileInstances = createTileInstances();

		glGenVertexArrays(1, &tileVao);
		glGenBuffers(1, &tileVbo);
		glGenBuffers(1, &tileInstanceVbo);

		{
			glBindVertexArray(tileVao);
			glBindBuffer(GL_ARRAY_BUFFER, tileVbo);
			glBufferData(GL_ARRAY_BUFFER, this->tileMesh.size() * sizeof(float), this->tileMesh.data(), GL_STATIC_DRAW);

			// layout(location = 0) in vec2 position;
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

			// Define instance attributes
			glBindBuffer(GL_ARRAY_BUFFER, tileInstanceVbo);
			glBufferData(GL_ARRAY_BUFFER, this->tileInstances.size() * sizeof(TileInstance), this->tileInstances.data(), GL_STATIC_DRAW);

			// layout(location = 1) in vec2 instancePosition;
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TileInstance), (void*)offsetof(TileInstance, position));
			glVertexAttribDivisor(1, 1);

			// layout(location = 2) in int type;
			glEnableVertexAttribArray(2);
			glVertexAttribIPointer(2, 1, GL_INT, sizeof(TileInstance), (void*)offsetof(TileInstance, type));
			glVertexAttribDivisor(2, 1);

			glBindVertexArray(0);
		}
	}


	glm::vec2 RenderSystem::calculateWorldDimensions(const int columns, const int rows, const float tileScale) noexcept {
		return {
			tileScale * sqrt(3.0f) * (columns + 0.5f),
			tileScale * 1.5f * (rows + 1.0f)
		};
	}

	void RenderSystem::renderMap() const noexcept {
		const glm::vec2 worldDimensions = calculateWorldDimensions();
		const glm::mat4 projection = glm::ortho(0.0f, worldDimensions.x, 0.0f, worldDimensions.y, -1.0f, 1.0f);

		tileShader.use()
			.setMat4("projection", projection);

		glBindVertexArray(tileVao);
		glDrawArraysInstanced(GL_TRIANGLES, 0, this->tileMesh.size(), this->tileInstances.size());
		glBindVertexArray(0);
	}

}
