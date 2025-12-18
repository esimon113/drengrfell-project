#include "renderBuildings.h"
#include "core/camera.h"
#include "utils/worldNodeMapper.h"
#include <unordered_set>
#include <GLFW/glfw3.h>

namespace df {

	RenderBuildingsSystem RenderBuildingsSystem::init(Window* window, Registry* registry, GameState& gamestate) noexcept {
		RenderBuildingsSystem self;

        self.window = window;
        self.registry = registry;
        self.gamestate = &gamestate;

        self.viewport.origin = glm::uvec2(0);
        self.viewport.size = self.window->getWindowExtent();

		self.buildingHoverShader = Shader::init(assets::Shader::buildingHover).value();
		self.buildingShadowShader = Shader::init(assets::Shader::buildingShadow).value();
		self.spriteShader = Shader::init(assets::Shader::sprite).value();
		self.settlementTexture = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT1);
		self.roadPreviewTexture = Texture::init(assets::Texture::DIRT_ROAD_DIAGONAL_UP);
		self.roadTexture = Texture::init(assets::Texture::PATH_ROAD_VERTICAL);
		
		// Load all settlement textures
		self.settlementTextures[0] = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT1);
		self.settlementTextures[1] = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT2);
		self.settlementTextures[2] = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT3);
		self.settlementTextures[3] = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT4);
		self.settlementTextures[4] = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT5);

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

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		return self;
	}

	void RenderBuildingsSystem::deinit() noexcept {
		buildingHoverShader.deinit();
		buildingShadowShader.deinit();
		spriteShader.deinit();
		settlementTexture.deinit();
		roadPreviewTexture.deinit();
		roadTexture.deinit();
		for (auto& tex : settlementTextures) {
			tex.deinit();
		}
	}

	void RenderBuildingsSystem::step(float /*dt*/) noexcept {
		float time = static_cast<float>(glfwGetTime());
		renderSettlements(time);
		renderRoads(time);
	}

	void RenderBuildingsSystem::reset() noexcept {
	}

	// TODO: combine into one function if there are more building types to preview
	// Then pass some kind of building descriptor as parameter to the new function
	// -> beware NOT to create a gigantic switch statement

	void RenderBuildingsSystem::renderSettlementPreview(const glm::vec2& worldPosition, bool active, float time) noexcept {
		if (!active) return;

		// Set viewport = game viewport
		glViewport(this->viewport.origin.x, this->viewport.origin.y, this->viewport.size.x, this->viewport.size.y);

		// camera for zoom-factor
		Camera& cam = registry->cameras.get(registry->getCamera());

		const auto map = this->gamestate->getMap();
		uint32_t columns = map.getMapWidth();
		uint32_t rows = map.getTileCount() / columns;
		const glm::vec2 worldDimensions = calculateWorldDimensions(columns, rows);

		// projection with zoom-factor
		const glm::mat4 projection = glm::ortho(
			cam.position.x, cam.position.x + worldDimensions.x / cam.zoom,
			cam.position.y, cam.position.y + worldDimensions.y / cam.zoom,
			-1.0f, 1.0f
		);
		const glm::mat4 view = glm::identity<glm::mat4>();

		const float settlementSize = 0.5f;
		const float shadowOffsetY = -0.15f;
		const float shadowScale = 1.4f; // shadow is a bit larger than actual settlement

		// render shadow first = below settlement-textrue
		glm::mat4 shadowModel = glm::identity<glm::mat4>();
		shadowModel = glm::translate(shadowModel, glm::vec3(worldPosition.x, worldPosition.y + shadowOffsetY, -0.01f)); // Slightly behind
		shadowModel = glm::scale(shadowModel, glm::vec3(settlementSize * shadowScale, settlementSize * shadowScale, 1.0f));

		this->settlementTexture.bind(0);
		this->buildingShadowShader.use()
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
		this->buildingHoverShader.use()
			.setMat4("view", view)
			.setMat4("projection", projection)
			.setMat4("model[0]", hoverModel)
			.setSampler("sprite", 0)
			.setVec3("fcolor", glm::vec3(1.0f, 1.0f, 1.0f))
			.setFloat("time", time);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}


	void RenderBuildingsSystem::renderRoadPreview(const glm::vec2& worldPosition, bool active, float time) noexcept {
		if (!active) return;

		glViewport(this->viewport.origin.x, this->viewport.origin.y, this->viewport.size.x, this->viewport.size.y);

		// camera for zoom-factor
		Camera& cam = registry->cameras.get(registry->getCamera());

		const auto map = this->gamestate->getMap();
		uint32_t columns = map.getMapWidth();
		uint32_t rows = map.getTileCount() / columns;
		const glm::vec2 worldDimensions = calculateWorldDimensions(columns, rows);

		// projection with zoom-factor
		const glm::mat4 projection = glm::ortho(
			cam.position.x, cam.position.x + worldDimensions.x / cam.zoom,
			cam.position.y, cam.position.y + worldDimensions.y / cam.zoom,
			-1.0f, 1.0f
		);
		const glm::mat4 view = glm::identity<glm::mat4>();

		const glm::vec2 baseRoadScale = glm::vec2(1.0f, 0.5f);
		const glm::vec2 roadScale = baseRoadScale * 1.7f;
		const float roadShadowOffsetY = -0.13f;
		const float roadShadowScale = 1.1f;

		// TODO: get shadow gradient to work (and look good, similar to settlement) -> wasted to much time on this for now
		glm::mat4 shadowModel = glm::identity<glm::mat4>();
		shadowModel = glm::translate(shadowModel, glm::vec3(worldPosition.x, worldPosition.y + roadShadowOffsetY, -0.01f));
		shadowModel = glm::scale(shadowModel, glm::vec3(roadScale * roadShadowScale, 1.0f));

		this->roadPreviewTexture.bind(0);
		this->buildingShadowShader.use()
			.setMat4("view", view)
			.setMat4("projection", projection)
			.setMat4("model[0]", shadowModel)
			.setSampler("sprite", 0);

		glBindVertexArray(this->m_quad_vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		// Road sprite pass
		glm::mat4 model = glm::identity<glm::mat4>();
		model = glm::translate(model, glm::vec3(worldPosition, 0.0f));
		model = glm::scale(model, glm::vec3(roadScale, 1.0f));

		this->roadPreviewTexture.bind(0);
		this->buildingHoverShader.use()
			.setMat4("view", view)
			.setMat4("projection", projection)
			.setMat4("model[0]", model)
			.setSampler("sprite", 0)
			.setVec3("fcolor", glm::vec3(1.0f))
			.setFloat("time", time);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}

	// Helper function to get vertex world position
	static glm::vec2 getVertexWorldPosition(size_t vertexId, const Graph& map) {
		const float hexagonRadius = 1.0f;
		const uint32_t columns = map.getMapWidth();
		
		// Find which tile(s) contain this vertex
		for (size_t tileId = 0; tileId < map.getTileCount(); ++tileId) {
			const Tile& tile = map.getTile(tileId);
			const auto vertices = map.getTileVertices(tile);
			
			for (size_t i = 0; i < vertices.size(); ++i) {
				if (vertices[i].getId() == vertexId) {
					uint32_t row = tileId / columns;
					uint32_t col = tileId % columns;
					glm::vec2 tileCenterPos = WorldNodeMapper::getTilePosition(row, col);
					std::array<glm::vec2, 6> vertexOffsets = WorldNodeMapper::getVertexOffsets(hexagonRadius);
					return tileCenterPos + vertexOffsets[i];
				}
			}
		}
		return glm::vec2(0.0f);
	}

	// Helper function to get edge world position
	static glm::vec2 getEdgeWorldPosition(size_t edgeId, const Graph& map) {
		const float hexagonRadius = 1.0f;
		const uint32_t columns = map.getMapWidth();
		
		// Find which tile(s) contain this edge
		for (size_t tileId = 0; tileId < map.getTileCount(); ++tileId) {
			const Tile& tile = map.getTile(tileId);
			const auto edges = map.getTileEdges(tile);
			
			for (size_t i = 0; i < edges.size(); ++i) {
				if (edges[i].getId() == edgeId) {
					uint32_t row = tileId / columns;
					uint32_t col = tileId % columns;
					glm::vec2 tileCenterPos = WorldNodeMapper::getTilePosition(row, col);
					std::array<glm::vec2, 6> vertexOffsets = WorldNodeMapper::getVertexOffsets(hexagonRadius);
					
					// Edge position is center between two neighboring vertices
					glm::vec2 vertex1Position = tileCenterPos + vertexOffsets[i];
					glm::vec2 vertex2Position = tileCenterPos + vertexOffsets[(i + 1) % 6];
					return (vertex1Position + vertex2Position) / 2.0f;
				}
			}
		}
		return glm::vec2(0.0f);
	}

	void RenderBuildingsSystem::renderSettlements(float /*time*/) noexcept {
		const Graph& map = this->gamestate->getMap();
		
		// Set viewport
		glViewport(this->viewport.origin.x, this->viewport.origin.y, this->viewport.size.x, this->viewport.size.y);

		// Camera for zoom-factor
		Camera& cam = registry->cameras.get(registry->getCamera());

		uint32_t columns = map.getMapWidth();
		uint32_t rows = map.getTileCount() / columns;
		const glm::vec2 worldDimensions = calculateWorldDimensions(columns, rows);

		// Projection with zoom-factor
		const glm::mat4 projection = glm::ortho(
			cam.position.x, cam.position.x + worldDimensions.x / cam.zoom,
			cam.position.y, cam.position.y + worldDimensions.y / cam.zoom,
			-1.0f, 1.0f
		);
		const glm::mat4 view = glm::identity<glm::mat4>();

		const float settlementSize = 0.5f;

		// Track processed vertices to avoid duplicates (shared vertices)
		std::unordered_set<size_t> processedVertices;

		// Iterate through all vertices and render those with settlements
		for (const auto& vertex : map.getVertices()) {
			size_t vertexId = vertex.getId();
			
			if (!vertex.hasSettlement() || processedVertices.find(vertexId) != processedVertices.end()) {
				continue;
			}
			processedVertices.insert(vertexId);

			glm::vec2 worldPosition = getVertexWorldPosition(vertexId, map);
			glm::vec2 pos = worldPosition - cam.position;

			// Render settlement (no shadow, no pulsing effect)
			glm::mat4 model = glm::identity<glm::mat4>();
			model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
			model = glm::scale(model, glm::vec3(settlementSize, settlementSize, 1.0f));

			// Use a settlement texture (cycle through them based on vertex ID for variety)
			size_t textureIndex = vertexId % 5;
			this->settlementTextures[textureIndex].bind(0);
			this->spriteShader.use()
				.setMat4("view", view)
				.setMat4("projection", projection)
				.setMat4("model[0]", model)
				.setSampler("sprite", 0)
				.setVec3("fcolor", glm::vec3(1.0f, 1.0f, 1.0f));

			glBindVertexArray(this->m_quad_vao);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		glBindVertexArray(0);
	}

	void RenderBuildingsSystem::renderRoads(float /*time*/) noexcept {
		const Graph& map = this->gamestate->getMap();
		
		// Set viewport
		glViewport(this->viewport.origin.x, this->viewport.origin.y, this->viewport.size.x, this->viewport.size.y);

		// Camera for zoom-factor
		Camera& cam = registry->cameras.get(registry->getCamera());

		uint32_t columns = map.getMapWidth();
		uint32_t rows = map.getTileCount() / columns;
		const glm::vec2 worldDimensions = calculateWorldDimensions(columns, rows);

		// Projection with zoom-factor
		const glm::mat4 projection = glm::ortho(
			cam.position.x, cam.position.x + worldDimensions.x / cam.zoom,
			cam.position.y, cam.position.y + worldDimensions.y / cam.zoom,
			-1.0f, 1.0f
		);
		const glm::mat4 view = glm::identity<glm::mat4>();

		const glm::vec2 baseRoadScale = glm::vec2(1.0f, 0.5f);
		const glm::vec2 roadScale = baseRoadScale * 1.7f;

		// Track processed edges to avoid duplicates (shared edges)
		std::unordered_set<size_t> processedEdges;

		// Iterate through all edges and render those with roads
		for (const auto& edge : map.getEdges()) {
			size_t edgeId = edge.getId();
			
			if (!edge.hasRoad() || processedEdges.find(edgeId) != processedEdges.end()) {
				continue;
			}
			processedEdges.insert(edgeId);

			glm::vec2 worldPosition = getEdgeWorldPosition(edgeId, map);
			glm::vec2 pos = worldPosition - cam.position;

			// Render road (no shadow, no pulsing effect)
			glm::mat4 model = glm::identity<glm::mat4>();
			model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
			model = glm::scale(model, glm::vec3(roadScale, 1.0f));

			this->roadTexture.bind(0);
			this->spriteShader.use()
				.setMat4("view", view)
				.setMat4("projection", projection)
				.setMat4("model[0]", model)
				.setSampler("sprite", 0)
				.setVec3("fcolor", glm::vec3(1.0f));

			glBindVertexArray(this->m_quad_vao);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		glBindVertexArray(0);
	}
}
