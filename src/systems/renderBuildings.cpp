#include "renderBuildings.h"
#include "GL/glcorearb.h"
#include "core/camera.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "systems/renderCommon.h"
#include <GLFW/glfw3.h>



namespace df {

	RenderBuildingsSystem RenderBuildingsSystem::init(Window* window, Registry* registry, std::shared_ptr<GameState> gameState) noexcept {
		RenderBuildingsSystem self;

		self.window = window;
		self.registry = registry;
		self.gamestate = gameState;

		self.viewport.origin = glm::uvec2(0);
		self.viewport.size = self.window->getWindowExtent();

		self.spriteShader = Shader::init(assets::Shader::sprite).value();
		self.roadTextureDiagonalUp = Texture::init(assets::Texture::DIRT_ROAD_DIAGONAL_UP);
		self.roadTextureDiagonalDown = Texture::init(assets::Texture::DIRT_ROAD_DIAGONAL_DOWN);
		self.roadTextureVertical = Texture::init(assets::Texture::DIRT_ROAD_VERTICAL);

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
		spriteShader.deinit();
		roadTextureDiagonalUp.deinit();
		roadTextureDiagonalDown.deinit();
		roadTextureVertical.deinit();

		for (auto& tex : settlementTextures) {
			tex.deinit();
		}
	}


	void RenderBuildingsSystem::step(float /*dt*/) noexcept {
		float time = static_cast<float>(glfwGetTime());
		renderBuildings(time);
	}


	void RenderBuildingsSystem::reset() noexcept {}


	const glm::mat4 RenderBuildingsSystem::calculateProjection(const Camera& cam) const {
		glm::uvec2 extent = window->getWindowExtent();
		glViewport(0, 0, extent.x, extent.y);

		return glm::ortho(
			cam.minX(), cam.maxX(),
			cam.minY(), cam.maxY(),
			-1.0f, 1.0f
		);
	}


	void RenderBuildingsSystem::renderBuildings(float time) noexcept {
		if (!registry || !gamestate) return;

		glBindVertexArray(m_quad_vao);
		Camera& cam = registry->cameras.get(registry->getCamera());

		const glm::mat4 view = glm::identity<glm::mat4>();
		const glm::mat4 projection = this->calculateProjection(cam);

		// TODO: use consistent FPS for animations
		constexpr float animationSpeed = 5.0f; // fps
		constexpr int numFrames = 5; // how many frames per animation run
		int textureIndex = static_cast<int>(time * animationSpeed) % numFrames;

		// Render settlements from ECS
		for (Entity e : registry->settlements.entities) {
			if (!registry->positions.has(e) || !registry->scales.has(e)) continue;

			// const Settlement& settlement = registry->settlements.get(e);
			const glm::vec2& worldPos = registry->positions.get(e);
			const glm::vec2& scale = registry->scales.get(e);

			glm::mat4 model = glm::identity<glm::mat4>();
			model = glm::translate(model, glm::vec3(worldPos, 0.0f));
			model = glm::scale(model, glm::vec3(scale, 1.0f));

			// Use animated settlement texture
			settlementTextures[textureIndex].bind(0);
			spriteShader.use()
				.setMat4("view", view)
				.setMat4("model[0]", model)
				.setMat4("projection", projection)
				.setSampler("sprite", 0)
				.setVec3("fcolor", glm::vec3(1.0f));

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		// Render roads from ECS
		for (Entity e : registry->roads.entities) {
			if (!registry->positions.has(e) || !registry->scales.has(e)) continue;

			const glm::vec2& worldPos = registry->positions.get(e);
			const glm::vec2& scale = registry->scales.get(e);

			int edgeIndex = this->registry->roadEdgeIndices.has(e) ? this->registry->roadEdgeIndices.get(e) : -1;

			Texture* roadTexture = nullptr;
			if (edgeIndex == 0 || edgeIndex == 3) roadTexture = &roadTextureDiagonalDown;
			else if (edgeIndex == 2 || edgeIndex == 5) roadTexture = &roadTextureDiagonalUp;
			else if (edgeIndex == 1 || edgeIndex == 4) roadTexture = &roadTextureVertical;
			else roadTexture = &roadTextureVertical;

			glm::mat4 model = glm::identity<glm::mat4>();
			model = glm::translate(model, glm::vec3(worldPos, 0.0f));
			model = glm::scale(model, glm::vec3(scale, 1.0f));

			assert(roadTexture != nullptr); // should not fail
			roadTexture->bind(0);
			spriteShader.use()
				.setMat4("model[0]", model)
				.setMat4("view", view)
				.setMat4("projection", projection)
				.setSampler("sprite", 0)
				.setVec3("fcolor", glm::vec3(1.0f));

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		glBindVertexArray(0);
	}
}
