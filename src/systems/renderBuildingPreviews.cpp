#include "renderBuildingPreviews.h"
#include "GL/glcorearb.h"
#include "core/camera.h"
#include "core/components.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "systems/renderCommon.h"
#include <GLFW/glfw3.h>



namespace df {

	RenderBuildingPreviewsSystem RenderBuildingPreviewsSystem::init(Window* window, Registry* registry, std::shared_ptr<GameState> gameState) noexcept {
		RenderBuildingPreviewsSystem self;

		self.window = window;
		self.registry = registry;
		self.gamestate = gameState;

		self.viewport.origin = glm::uvec2(0);
		self.viewport.size = self.window->getWindowExtent();

		self.buildingHoverShader = Shader::init(assets::Shader::buildingHover).value();
		self.buildingShadowShader = Shader::init(assets::Shader::buildingShadow).value();

		// Load all settlement textures for animation
		self.settlementTextures[0] = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT1);
		self.settlementTextures[1] = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT2);
		self.settlementTextures[2] = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT3);
		self.settlementTextures[3] = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT4);
		self.settlementTextures[4] = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT5);
		self.roadPreviewTexture = Texture::init(assets::Texture::DIRT_ROAD_DIAGONAL_UP);

		glm::uvec2 extent = self.window->getWindowExtent();
		self.intermediateFramebuffer = Framebuffer::init({static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), 1, true});

		float quadVertices[] = {
			// positions (centered at origin)	// texcoords
			-0.5f, -0.5f, 0.0f, 0.0f,
			0.5f, -0.5f, 1.0f, 0.0f,
			0.5f, 0.5f, 1.0f, 1.0f,
			-0.5f, 0.5f, 0.0f, 1.0f};
		constexpr GLuint quadIndices[] = {0, 1, 2, 2, 3, 0};

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


	void RenderBuildingPreviewsSystem::deinit() noexcept {
		buildingHoverShader.deinit();
		buildingShadowShader.deinit();
		for (auto& tex : settlementTextures)
			tex.deinit();
		roadPreviewTexture.deinit();
		intermediateFramebuffer.deinit();
	}


	void RenderBuildingPreviewsSystem::step(float /*dt*/) noexcept {
		float time = static_cast<float>(glfwGetTime());
		renderPreviews(time);
	}


	void RenderBuildingPreviewsSystem::reset() noexcept {}


	const glm::mat4 RenderBuildingPreviewsSystem::calculateProjection(const Camera& cam) const {
		glm::uvec2 extent = window->getWindowExtent();
		glViewport(0, 0, extent.x, extent.y);

		return glm::ortho(
			0.0f, cam.viewWidth,
			0.0f, cam.viewHeight,
			-1.0f, 1.0f);
	}


	void RenderBuildingPreviewsSystem::renderPreviews(float time) noexcept {
		if (!registry || !window || !gamestate)
			return;

		Camera& cam = registry->cameras.get(registry->getCamera());

		const glm::mat4 view = glm::identity<glm::mat4>();
		const glm::mat4 projection = this->calculateProjection(cam);

		glBindVertexArray(m_quad_vao);

		// Render building previews from ECS
		for (Entity e : registry->buildingPreviews.entities) {
			if (!registry->scales.has(e) || !registry->positions.has(e))
				continue;

			const BuildingPreviewComponent& preview = registry->buildingPreviews.get(e);
			const glm::vec2& scale = registry->scales.get(e);
			const glm::vec2& pos = registry->positions.get(e);

			if (preview.type == BuildingPreviewType::Settlement) { // settlement
				const float shadowOffsetY = -0.15f;
				const float shadowScale = 1.4f;

				// TODO: use consistent FPS for animations
				constexpr float animationSpeed = 5.0f; // fps
				constexpr int numFrames = 5;		   // how many frames per animation run
				int textureIndex = static_cast<int>(time * animationSpeed) % numFrames;

				// Render shadow
				glm::mat4 shadowModel = glm::identity<glm::mat4>();
				shadowModel = glm::translate(shadowModel, glm::vec3(pos.x, pos.y + shadowOffsetY, -0.01f));
				shadowModel = glm::scale(shadowModel, glm::vec3(scale.x * shadowScale, scale.y * shadowScale, 1.0f));

				settlementTextures[textureIndex].bind(0);
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

				settlementTextures[textureIndex].bind(0);
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
} // namespace df
