#include "render.h"
#include "../core/player.h"
#include "../core/tile.h"
#include "utils/textureArray.h"


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
		self.tileAtlas = TextureArray::init(assets::Texture::TILE_ATLAS, static_cast<int>(df::types::TileType::COUNT), 60, 59);

		self.settlementHoverShader = Shader::init(assets::Shader::settlementHover).value();
		self.settlementShadowShader = Shader::init(assets::Shader::settlementShadow).value();
		self.settlementTexture = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT1);

		glm::uvec2 extent = self.window->getWindowExtent();
		self.intermediateFramebuffer = Framebuffer::init({ static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), 1, true });

		glGenVertexArrays(1, &self.m_quad_vao);
		glBindVertexArray(self.m_quad_vao);
		glGenBuffers(1, &self.m_quad_ebo);

		constexpr ::std::array<GLuint, 6> indices = { 0, 1, 2, 2, 3, 0 };
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self.m_quad_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		self.initMap();

		return self;
	}


	void RenderSystem::deinit() noexcept {
		spriteShader.deinit();
		windShader.deinit();
		tileShader.deinit();
		tileAtlas.deinit();

		settlementHoverShader.deinit();
		settlementShadowShader.deinit();
		settlementTexture.deinit();
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
	std::vector<float> RenderSystem::createTileMesh() noexcept {
		// Appends the hexagons corners counter-clockwise to the vertices array.
		// The center of the hexagon is at the origin.
		// It is rotated by 30 degrees in order to have a corner at the top,
		// as the tile textures already created have also the corner at the top.

		constexpr float SQRT_3_DIV_2 = 0.866025404f; //sqrt(3.0) / 2.0f;
		std::vector<TileVertex> vertices;
		for (int vertex = 0; vertex < 6; vertex++) {
			const float angle = M_PI / 180.0f * (60.0f * static_cast<float>(vertex) - 30.0f);
			float x = std::cos(angle);
			float y = std::sin(angle);
			float u = (x + SQRT_3_DIV_2) / (2.0f * SQRT_3_DIV_2);
			float v = (y + 1.0f) / 2.0f;
			vertices.emplace_back(glm::vec2(x, y), glm::vec2(u, v));
		}

		// This is a triangulation I've come up with on my ipad.
		// The triangles are counter-clockwise as the vertices above
		// are counter-clockwise around the origin.

		std::vector<float> meshData;

		// Big center triangle
		for (int i = 0; i < 6; i += 2) {
			meshData.push_back(vertices[i].position.x);
			meshData.push_back(vertices[i].position.y);
			meshData.push_back(vertices[i].uv.x);
			meshData.push_back(vertices[i].uv.y);
		}

		// Three side triangles
		for (int i = 0; i < 3; i++) {
			meshData.push_back(vertices[2 * i + 0].position.x);
			meshData.push_back(vertices[2 * i + 0].position.y);
			meshData.push_back(vertices[2 * i + 0].uv.x);
			meshData.push_back(vertices[2 * i + 0].uv.y);

			meshData.push_back(vertices[2 * i + 1].position.x);
			meshData.push_back(vertices[2 * i + 1].position.y);
			meshData.push_back(vertices[2 * i + 1].uv.x);
			meshData.push_back(vertices[2 * i + 1].uv.y);

			meshData.push_back(vertices[(2 * i + 2) % 6].position.x);
			meshData.push_back(vertices[(2 * i + 2) % 6].position.y);
			meshData.push_back(vertices[(2 * i + 2) % 6].uv.x);
			meshData.push_back(vertices[(2 * i + 2) % 6].uv.y);
		}

		return meshData;
	}

	std::vector<float> RenderSystem::createRectangularTileMesh() noexcept {
		std::vector<TileVertex> vertices;
		vertices.push_back({{1, -1}, {1, 0}});
		vertices.push_back({{1, 1}, {1, 1}});
		vertices.push_back({{-1, 1}, {0, 1}});
		vertices.push_back({{-1, -1}, {0, 0}});

		std::vector<float> meshData;
		for (int i = 0; i < 2; i++) {
			meshData.push_back(vertices[2 * i + 0].position.x);
			meshData.push_back(vertices[2 * i + 0].position.y);
			meshData.push_back(vertices[2 * i + 0].uv.x);
			meshData.push_back(vertices[2 * i + 0].uv.y);

			meshData.push_back(vertices[2 * i + 1].position.x);
			meshData.push_back(vertices[2 * i + 1].position.y);
			meshData.push_back(vertices[2 * i + 1].uv.x);
			meshData.push_back(vertices[2 * i + 1].uv.y);

			meshData.push_back(vertices[(2 * i + 2) % 4].position.x);
			meshData.push_back(vertices[(2 * i + 2) % 4].position.y);
			meshData.push_back(vertices[(2 * i + 2) % 4].uv.x);
			meshData.push_back(vertices[(2 * i + 2) % 4].uv.y);
		}


		return meshData;
	}

	/**
	 * Creates a vector of the tile-specific data for the whole map arranged in a "rectangle".
	 * The tile specific data is the position and tile type, yet.
	 */
	std::vector<RenderSystem::TileInstance> RenderSystem::createTileInstances(const int columns, const int rows) noexcept {
		// TODO: Add dedicated world generator.
		auto randomEngine = std::default_random_engine(std::random_device()());
		auto uniformTileTypeDistribution = std::uniform_int_distribution(1, static_cast<int>(df::types::TileType::COUNT) - 1);

		std::vector<TileInstance> instances;
		for (int row = rows - 1; row >= 0; row--) {
			for (int column = 0; column < columns; column++) {
				glm::vec2 position;
				position.x = 2.0f * (column + 0.5f * (row & 1));
				position.y = row * 1.5f;
				const int type = uniformTileTypeDistribution(randomEngine);

				instances.push_back({position, type, 0, uniformTileTypeDistribution(randomEngine) % 6 > 2});
			}
		}
		return instances;
	}

	/**
	 * Initializes the map data
	 */
	void RenderSystem::initMap() noexcept {
		this->tileMesh = createRectangularTileMesh();
		this->tileInstances = createTileInstances(10, 10);

		glGenVertexArrays(1, &tileVao);
		glGenBuffers(1, &tileVbo);
		glGenBuffers(1, &tileInstanceVbo);

		{
			glBindVertexArray(tileVao);

			tileInstances[0].explored = 1; // for testing
			tileInstances[10].explored = 1; // for testing

			glBindBuffer(GL_ARRAY_BUFFER, tileVbo);
			glBufferData(GL_ARRAY_BUFFER, this->tileMesh.size() * sizeof(float), this->tileMesh.data(), GL_STATIC_DRAW);

			// layout(location = 0) in vec2 position;
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, position));

			// layout(location = 1) in vec2 vertexUv;
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, uv));

			// Define instance attributes
			glBindBuffer(GL_ARRAY_BUFFER, tileInstanceVbo);
			glBufferData(GL_ARRAY_BUFFER, this->tileInstances.size() * sizeof(TileInstance), this->tileInstances.data(), GL_STATIC_DRAW);

			// layout(location = 2) in vec2 instancePosition;
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TileInstance), (void*)offsetof(TileInstance, position));
			glVertexAttribDivisor(2, 1);

			// layout(location = 3) in int type;
			glEnableVertexAttribArray(3);
			glVertexAttribIPointer(3, 1, GL_INT, sizeof(TileInstance), (void*)offsetof(TileInstance, type));
			glVertexAttribDivisor(3, 1);

			// layout(location = 4) in int explored;
			glEnableVertexAttribArray(4);
			glVertexAttribIPointer(4, 1, GL_INT, sizeof(TileInstance), (void*)offsetof(TileInstance, explored));
			glVertexAttribDivisor(4, 1);

			glBindVertexArray(0);
		}
	}


	glm::vec2 RenderSystem::calculateWorldDimensions(const int columns, const int rows) noexcept {
		return {
			sqrt(3.0f) * (columns + 0.5f),
			1.5f * (rows + 1.0f)
		};
	}

	void RenderSystem::renderMap(const glm::vec2 scale) const noexcept {
		const glm::vec2 worldDimensions = calculateWorldDimensions(10, 10);
		const glm::mat4 projection = glm::ortho(0.0f, worldDimensions.x, 0.0f, worldDimensions.y, -1.0f, 1.0f);

		glm::mat4 model = glm::identity<glm::mat4>();
		model = glm::scale(model, glm::vec3(scale, 1));

		tileAtlas.bind(0);
		tileShader.use()
			.setMat4("model", model)
			.setMat4("projection", projection)
			.setSampler("tileAtlas", 0);

		glBindVertexArray(tileVao);
		glDrawArraysInstanced(GL_TRIANGLES, 0, this->tileMesh.size() / FLOATS_PER_TILE_VERTEX, this->tileInstances.size());
		glBindVertexArray(0);
	}

	void RenderSystem::updateFogOfWar(const Player* player) noexcept {
		if (player == nullptr) return;

		for(auto& instance : tileInstances) {
			instance.explored = 0;
		}

		const auto& exploredTiles = player->getExploredTiles();
    	for (const Tile* tile : exploredTiles) {
        	if (tile == nullptr) continue;
        
        	size_t tileId = tile->getId();
        	if (tileId < tileInstances.size()) {
        	    tileInstances[tileId].explored = 1;
        	}
    	}

		glBindBuffer(GL_ARRAY_BUFFER, tileInstanceVbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, 
						tileInstances.size() * sizeof(TileInstance), 
						tileInstances.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}


	// Converts screen coordinates to world coordinates
	glm::vec2 RenderSystem::screenToWorldCoordinates(const glm::vec2& screenPos) const noexcept {
	    const glm::vec2 worldDimensions = calculateWorldDimensions(10, 10);

		glm::vec2 viewportPos = screenPos - glm::vec2(this->m_viewport.m_origin);
		glm::vec2 normalizedPos = viewportPos / glm::vec2(this->m_viewport.m_size);
		normalizedPos.y = 1.0f - normalizedPos.y; // flip y: screen-y increases downwards, world-y up

		return normalizedPos * worldDimensions;
	}


	void RenderSystem::renderSettlementPreview(const glm::vec2& worldPosition, bool active, float time) noexcept {
		if (!active) return;

		// Set viewport = game viewport
		glViewport(this->m_viewport.m_origin.x, this->m_viewport.m_origin.y, this->m_viewport.m_size.x, this->m_viewport.m_size.y);

		const glm::vec2 worldDimensions = calculateWorldDimensions(10, 10);
		const glm::mat4 projection = glm::ortho(0.0f, worldDimensions.x, 0.0f, worldDimensions.y, -1.0f, 1.0f);
		const glm::mat4 view = glm::identity<glm::mat4>();

		const float settlementSize = 0.5f;
		const float shadowOffsetY = -0.15f;
		const float shadowScale = 1.4f; // shadow is a bit larger than actual settlement

		// render shadow first = below settlement-textrue
		glm::mat4 shadowModel = glm::identity<glm::mat4>();
		shadowModel = glm::translate(shadowModel, glm::vec3(worldPosition.x, worldPosition.y + shadowOffsetY, -0.01f)); // Slightly behind
		shadowModel = glm::scale(shadowModel, glm::vec3(settlementSize * shadowScale, settlementSize * shadowScale, 1.0f));

		this->settlementTexture.bind(0);
		this->settlementShadowShader.use()
			.setMat4("view", view)
			.setMat4("projection", projection)
			.setMat4("model[0]", shadowModel)
			.setSampler("sprite", 0);

		glBindVertexArray(this->m_quad_vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		// render settlement on top of shadow
		glm::mat4 hoverModel = glm::identity<glm::mat4>();
		hoverModel = glm::translate(hoverModel, glm::vec3(worldPosition, 0.0f));
		hoverModel = glm::scale(hoverModel, glm::vec3(settlementSize, settlementSize, 1.0f));

		this->settlementTexture.bind(0);
		this->settlementHoverShader.use()
			.setMat4("view", view)
			.setMat4("projection", projection)
			.setMat4("model[0]", hoverModel)
			.setSampler("sprite", 0)
			.setVec3("fcolor", glm::vec3(1.0f, 1.0f, 1.0f))
			.setFloat("time", time);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}

}
