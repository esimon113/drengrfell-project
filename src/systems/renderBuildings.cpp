#include "renderBuildings.h"
#include "core/camera.h"

namespace df {

	RenderBuildingsSystem RenderBuildingsSystem::init(Window* window, Registry* registry, std::shared_ptr<GameState> gameState) noexcept {
		RenderBuildingsSystem self;

        self.window = window;
        self.registry = registry;
        self.gamestate = gameState;

        self.viewport.origin = glm::uvec2(0);
        self.viewport.size = self.window->getWindowExtent();

		self.buildingHoverShader = Shader::init(assets::Shader::buildingHover).value();
		self.buildingShadowShader = Shader::init(assets::Shader::buildingShadow).value();
		self.settlementTexture = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT1);
		self.roadPreviewTexture = Texture::init(assets::Texture::DIRT_ROAD_DIAGONAL_UP);

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
		settlementTexture.deinit();
		roadPreviewTexture.deinit();
	}

	void RenderBuildingsSystem::step(float /*dt*/) noexcept {
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
}
