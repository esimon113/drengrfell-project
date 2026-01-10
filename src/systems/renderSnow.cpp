#include "renderSnow.h"
#include "../core/camera.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

/*
 *   Used the OPENGL particle system tutorial :
 *   https://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/particles-instancing/
 *   to create the particles aand then render it as snow
 */

namespace df {

	// The VBO containing the 4 vertices of the particles (billboard quad)
	static const GLfloat g_vertex_buffer_data[] = {
		-0.5f,
		-0.5f,
		0.0f,
		0.5f,
		-0.5f,
		0.0f,
		-0.5f,
		0.5f,
		0.0f,
		0.5f,
		0.5f,
		0.0f,
	};

	void RenderSnowSystem::reset() {}

	RenderSnowSystem RenderSnowSystem::init(Window* window, Registry* registry, std::shared_ptr<GameState> gamestate) noexcept {
		RenderSnowSystem self;

		self.window = window;
		self.registry = registry;
		self.gameState = gamestate;

		self.maxParticles = 50000;
		self.particlesCount = 0;

		self.particlesContainer.resize(self.maxParticles);
		self.g_particule_position_size_data.resize(self.maxParticles * 4);
		self.g_particule_color_data.resize(self.maxParticles * 4);

		for (int i = 0; i < self.maxParticles; i++) {
			self.particlesContainer[i].life = -1.0f;
		}

		self.particleShader = Shader::init(assets::Shader::particle).value();

		self.initBuffers();

		return self;
	}

	void RenderSnowSystem::initBuffers() noexcept {
		// Create VAO
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		// The VBO containing the 4 vertices of the particles (shared by all particles)
		glGenBuffers(1, &billboard_vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

		// The VBO containing the positions and sizes of the particles
		glGenBuffers(1, &particles_position_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
		// Initialize with empty (NULL) buffer : it will be updated later, each frame.
		glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

		// The VBO containing the colors of the particles
		glGenBuffers(1, &particles_color_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
		// Initialize with empty (NULL) buffer : it will be updated later, each frame.
		glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);

		glBindVertexArray(0);
	}

	void RenderSnowSystem::deinit() noexcept {
		glDeleteBuffers(1, &billboard_vertex_buffer);
		glDeleteBuffers(1, &particles_position_buffer);
		glDeleteBuffers(1, &particles_color_buffer);
		glDeleteVertexArrays(1, &vao);

		particleShader.deinit();
	}

	int RenderSnowSystem::findUnusedParticle() noexcept {
		static int lastUsedParticle = 0;
		for (int i = lastUsedParticle; i < maxParticles; i++) {
			if (particlesContainer[i].life < 0) {
				lastUsedParticle = i;
				return i;
			}
		}

		for (int i = 0; i < lastUsedParticle; i++) {
			if (particlesContainer[i].life < 0) {
				lastUsedParticle = i;
				return i;
			}
		}

		return 0; // All particles are taken, override the first one
	}

	void RenderSnowSystem::step(float deltaTime) noexcept {
		Camera& cam = registry->cameras.get(registry->getCamera());

		int rows = RenderCommon::getMapRows<int>(this->gameState->getMap());
		int cols = RenderCommon::getMapColumns<int>(this->gameState->getMap());

		const glm::vec2 worldDims = calculateWorldDimensions(cols, rows);

		float visibleWidth  = worldDims.x / cam.zoom;
		float visibleHeight = worldDims.y / cam.zoom;

		float padding = 14.0f;

		float spawnLeft   = cam.position.x - padding;
		float spawnRight  = cam.position.x + visibleWidth + padding;
		float spawnBottom = cam.position.y - padding;
		float spawnTop    = cam.position.y + visibleHeight + padding;

		static float lastZoom = cam.zoom;
		bool zoomOut = cam.zoom < lastZoom;
		lastZoom = cam.zoom;

		float visibleArea = visibleWidth * visibleHeight;
		float density = 0.017f;
		int minParticles = rows/2;
		int newparticles;
		if(rows > 6){
			newparticles = visibleArea * density * 0.05f;
		} else {
			newparticles = minParticles;
		}
		


		if (zoomOut) {
			newparticles *= 2;
		}

		for (int i = 0; i < newparticles; i++) {
			int unParticles = findUnusedParticle();
			Particle& p = particlesContainer[unParticles];

			float rx = static_cast<float>(rand()) / RAND_MAX;
			float ry = static_cast<float>(rand()) / RAND_MAX;
			float rz = static_cast<float>(rand()) / RAND_MAX;

			p.depth = rz* rz;

			p.pos = glm::vec3(
				spawnLeft + rx*(spawnRight - spawnLeft),
				spawnBottom+ ry* (spawnTop - spawnBottom),
				0.0f
			);



			float baseFall = -1.8f;
			float depthFall = -1.6f;

			p.speed.y = baseFall + p.depth * depthFall;
			p.speed.x = ((rand() % 60 - 30) / 600.0f) * (0.3f + p.depth);

			float fallDistance = visibleHeight + padding * 2.0f;
			p.life = fallDistance / std::abs(p.speed.y);

			// new size with added depth
			p.size = 0.02f + p.depth * (rows * 0.016f);

			p.r = 235;
			p.g = 238;
			p.b = 242;
			p.a = 50 + p.depth * 120;
		}

		particlesCount = 0;

		for (int i = 0; i < maxParticles; i++) {
			Particle& p = particlesContainer[i];

			if (p.life > 0.0f) {
				p.life -= deltaTime;
				p.pos += p.speed * deltaTime;

				p.pos.x += sin(p.life*2.0f) * 0.06f * (0.2f + p.depth)* deltaTime;

				if (p.pos.x < spawnLeft  || p.pos.x > spawnRight ||
					p.pos.y < spawnBottom) {
					p.life = -1.0f;
					continue;
				}

				g_particule_position_size_data[4 * particlesCount + 0] = p.pos.x;
				g_particule_position_size_data[4 * particlesCount + 1] = p.pos.y;
				g_particule_position_size_data[4 * particlesCount + 2] = p.pos.z;
				g_particule_position_size_data[4 * particlesCount + 3] = p.size;

				g_particule_color_data[4 * particlesCount + 0] = p.r;
				g_particule_color_data[4 * particlesCount + 1] = p.g;
				g_particule_color_data[4 * particlesCount + 2] = p.b;
				g_particule_color_data[4 * particlesCount + 3] = p.a;

				particlesCount++;
			}
		}

		const glm::mat4 projection = glm::ortho(
			cam.position.x,
			cam.position.x + visibleWidth,
			cam.position.y,
			cam.position.y + visibleHeight,
			-1.0f,
			1.0f
		);

		render(glm::mat4(1.0f), projection);
	}



	void RenderSnowSystem::render(const glm::mat4& view, const glm::mat4& projection) noexcept {
		if (particlesCount == 0)
			return;


		// Update buffers
		glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
		glBufferSubData(GL_ARRAY_BUFFER, 0, particlesCount * sizeof(GLfloat) * 4, g_particule_position_size_data.data());

		glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
		glBufferSubData(GL_ARRAY_BUFFER, 0, particlesCount * sizeof(GLubyte) * 4, g_particule_color_data.data());

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		particleShader.use();
		particleShader.setMat4("V", view);
		particleShader.setMat4("P", projection);

		glBindVertexArray(vao);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
		glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);

		glVertexAttribDivisor(0, 0);
		glVertexAttribDivisor(1, 1);
		glVertexAttribDivisor(2, 1);

		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, particlesCount);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);


		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glUseProgram(0);
	}

} // namespace df
