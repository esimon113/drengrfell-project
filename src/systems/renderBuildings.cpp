#include "renderBuildings.h"
#include "core/camera.h"
#include "core/components.h"
#include "glm/ext/vector_uint2.hpp"
#include "systems/renderCommon.h"
#include <GLFW/glfw3.h>
#include <cstdint>



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
			// positions (centered at origin)	// texcoords
			-0.5f, -0.5f,	0.0f, 0.0f,
			0.5f, -0.5f,	1.0f, 0.0f,
			0.5f, 0.5f,		1.0f, 1.0f,
			-0.5f, 0.5f,	0.0f, 1.0f
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
		renderPreviews(time);
		renderBuildings(time);
	}


	void RenderBuildingsSystem::reset() noexcept {}


	const glm::mat4 RenderBuildingsSystem::calculateProjection(const Camera& cam) const {
		const Graph& map = this->gamestate->getMap();
		const uint32_t columns = map.getMapWidth();
		const uint32_t rows = map.getTileCount() / columns;

		glm::uvec2 extent = window->getWindowExtent();
		glViewport(0, 0, extent.x, extent.y);
		const glm::vec2 worldDimensions = calculateWorldDimensions(columns, rows);

		return glm::ortho(
			cam.position.x, cam.position.x + worldDimensions.x / cam.zoom,
			cam.position.y, cam.position.y + worldDimensions.y / cam.zoom,
			-1.0f, 1.0f
		);
	}


	void RenderBuildingsSystem::renderBuildings(float /*time*/) noexcept {
		if (!registry || !gamestate) return;

		glBindVertexArray(m_quad_vao);

		const glm::mat4 view = glm::identity<glm::mat4>();
		Camera& cam = registry->cameras.get(registry->getCamera());
		const glm::mat4 projection = this->calculateProjection(cam);

		// Render settlements from ECS
		for (Entity e : registry->settlements.entities) {
			if (!registry->positions.has(e) || !registry->scales.has(e)) continue;

			const Settlement& settlement = registry->settlements.get(e);
			const glm::vec2& worldPos = registry->positions.get(e);
			const glm::vec2& scale = registry->scales.get(e);

			// Use camera-relative coordinates
			glm::vec2 pos = worldPos - cam.position;

			glm::mat4 model = glm::identity<glm::mat4>();
			model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
			model = glm::scale(model, glm::vec3(scale, 1.0f));

			// Use a settlement texture -> TODO: use for animation
			// currently a different frame is used for each placed settlemetn
			size_t textureIndex = settlement.getId() % 5;
			settlementTextures[textureIndex].bind(0);
			spriteShader.use()
				.setMat4("view", view)
				.setMat4("projection", projection)
				.setMat4("model[0]", model)
				.setSampler("sprite", 0)
				.setVec3("fcolor", glm::vec3(1.0f, 1.0f, 1.0f));

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		// Render roads from ECS
		for (Entity e : registry->roads.entities) {
			if (!registry->positions.has(e) || !registry->scales.has(e)) continue;

			const glm::vec2& worldPos = registry->positions.get(e);
			const glm::vec2& scale = registry->scales.get(e);

			// Use camera-relative coordinates
			glm::vec2 pos = worldPos - cam.position;

			glm::mat4 model = glm::identity<glm::mat4>();
			model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
			model = glm::scale(model, glm::vec3(scale, 1.0f));

			roadTexture.bind(0);
			spriteShader.use()
				.setMat4("view", view)
				.setMat4("projection", projection)
				.setMat4("model[0]", model)
				.setSampler("sprite", 0)
				.setVec3("fcolor", glm::vec3(1.0f));

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		glBindVertexArray(0);
	}


	void RenderBuildingsSystem::renderPreviews(float time) noexcept {
		if (!registry || !window || !gamestate) return;

		const glm::mat4 view = glm::identity<glm::mat4>();
		Camera& cam = registry->cameras.get(registry->getCamera());
		const glm::mat4 projection = this->calculateProjection(cam);

		glBindVertexArray(m_quad_vao);

		// Render building previews from ECS
		for (Entity e : registry->buildingPreviews.entities) {
			if (!registry->scales.has(e) || !registry->positions.has(e)) continue;

			const BuildingPreviewComponent& preview = registry->buildingPreviews.get(e);
			const glm::vec2& scale = registry->scales.get(e);
			const glm::vec2& pos = registry->positions.get(e);

			if (preview.type == BuildingPreviewType::Settlement) { // settlement
				const float shadowOffsetY = -0.15f;
				const float shadowScale = 1.4f;

				// Render shadow
				glm::mat4 shadowModel = glm::identity<glm::mat4>();
				shadowModel = glm::translate(shadowModel, glm::vec3(pos.x, pos.y + shadowOffsetY, -0.01f));
				shadowModel = glm::scale(shadowModel, glm::vec3(scale.x * shadowScale, scale.y * shadowScale, 1.0f));

				settlementTexture.bind(0);
				buildingShadowShader.use()
					.setMat4("view", view)
					.setMat4("projection", projection)
					.setMat4("model[0]", shadowModel)
					.setSampler("sprite", 0);

				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

				// Render settlement preview
				glm::mat4 model = glm::identity<glm::mat4>();
				model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
				model = glm::scale(model, glm::vec3(scale, 1.0f));

				settlementTexture.bind(0);
				buildingHoverShader.use()
					.setMat4("view", view)
					.setMat4("projection", projection)
					.setMat4("model[0]", model)
					.setSampler("sprite", 0)
					.setVec3("fcolor", glm::vec3(1.0f, 1.0f, 1.0f))
					.setFloat("time", time);
				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			} else if (preview.type == BuildingPreviewType::Road) { // road
				const float roadShadowOffsetY = -0.1f;
				const float roadShadowScale = 1.1f;

				// Render shadow -> TODO: road shadow needs some refinement
				glm::mat4 shadowModel = glm::identity<glm::mat4>();
				shadowModel = glm::translate(shadowModel, glm::vec3(pos.x, pos.y + roadShadowOffsetY, -0.01f));
				shadowModel = glm::scale(shadowModel, glm::vec3(scale * roadShadowScale, 1.0f));

				roadPreviewTexture.bind(0);
				buildingShadowShader.use()
					.setMat4("view", view)
					.setMat4("projection", projection)
					.setMat4("model[0]", shadowModel)
					.setSampler("sprite", 0);

				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

				// Render road preview
				glm::mat4 model = glm::identity<glm::mat4>();
				model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
				model = glm::scale(model, glm::vec3(scale, 1.0f));

				roadPreviewTexture.bind(0);
				buildingHoverShader.use()
					.setMat4("view", view)
					.setMat4("projection", projection)
					.setMat4("model[0]", model)
					.setSampler("sprite", 0)
					.setVec3("fcolor", glm::vec3(1.0f))
					.setFloat("time", time);

				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			}
		}
		glBindVertexArray(0);
	}
}
