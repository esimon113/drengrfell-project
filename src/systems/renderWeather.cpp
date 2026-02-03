#include "renderWeather.h"
#include "../core/camera.h"
#include "renderTiles.h"
#include "../core/tile.h"
#include "../core/graph.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random> 
#include <cmath> 

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

	void RenderWeatherSystem::reset() {

		for (int i = 0; i < maxParticles; i++) {
			particlesContainer[i].life = -1.0f;
		}
		this->particlesCount = 0;
		this->weatherIntensity = 0.0f; 
	}
	
	RenderWeatherSystem RenderWeatherSystem::init(Window* window, Registry* registry, std::shared_ptr<GameState> gamestate) noexcept {
		RenderWeatherSystem self;

		self.window = window;
		self.registry = registry;
		self.gameState = gamestate;

		self.maxParticles = 30000;
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

	void RenderWeatherSystem::initBuffers() noexcept {
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

	void RenderWeatherSystem::deinit() noexcept {
		glDeleteBuffers(1, &billboard_vertex_buffer);
		glDeleteBuffers(1, &particles_position_buffer);
		glDeleteBuffers(1, &particles_color_buffer);
		glDeleteVertexArrays(1, &vao);

		particleShader.deinit();
	}

	void RenderWeatherSystem::randomizeWeather() noexcept {
		auto tileSystem = registry->getSystem<RenderTilesSystem>();
		static std::random_device rd;
    	static std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dis(0.0f, 1.0f);
		df::types::WeatherType previous = currentType;
		auto& map = gameState->getMap();
		auto& tiles = map.getTiles();		

		// MARKOV CHAIN to have individual probabilities
		float transitionMatrix[3][3] = {
		//	 SUNNY  RAIN   SNOW
			{0.70f, 0.20f, 0.10f}, // From Sunny
			{0.25f, 0.60f, 0.15f}, // From Rain
			{0.20f, 0.20f, 0.60f}  // From Snow
		};

		int currentRow = static_cast<int>(currentType);
		float roll = dis(gen);
		float cumulativeProbability = 0.0f;


		for (int nextCol = 0; nextCol < 3; nextCol++) {
			cumulativeProbability += transitionMatrix[currentRow][nextCol];
			if (roll <= cumulativeProbability) {
				currentType = static_cast<df::types::WeatherType>(nextCol);
				break;
			}
		}



		if (currentType == df::types::WeatherType::SUNNY) {

			// If the weather starts to be sunny
			if ( previous != df::types::WeatherType::SUNNY){
				for (auto& tilePtr : tiles) {
					tilePtr->updateEffect(currentType);
				}
			}

			if(previous != df::types::WeatherType::SNOW){
				tileSystem->updateTileAtlas(1);
			}
			reset();
			weatherIntensity = 0.0f;
		}
		else if (currentType == df::types::WeatherType::RAIN){ 

			if ( previous != df::types::WeatherType::RAIN){
				for (auto& tilePtr : tiles) {
					tilePtr->updateEffect(currentType);
				}
				reset();
				weatherIntensity = -0.6f;
			} else if (previous == df::types::WeatherType::RAIN){
				weatherIntensity -= 0.2f;
			} 

			tileSystem->updateTileAtlas(static_cast<int>(currentType));

			
		} else if (currentType == df::types::WeatherType::SNOW) {

			
			if ( previous != df::types::WeatherType::SNOW){
				for (auto& tilePtr : tiles) {
					tilePtr->updateEffect(currentType);
				}
				reset();
				weatherIntensity = 0.6f;
			} else if( previous == df::types::WeatherType::SNOW) {
				tileSystem->updateTileAtlas(static_cast<int>(currentType));
				weatherIntensity += 0.2f;
			}
		}
	}


	int RenderWeatherSystem::findUnusedParticle() noexcept {
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

	void RenderWeatherSystem::step(float deltaTime) noexcept {
	
		static std::random_device rd;
		static std::mt19937 gen(rd()); 
		static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

		const float screenWidth = 100.0f;
		const float screenHeight = 100.0f;
		static float spawnAccumulator = 0.0f; 

		if (currentType == df::types::WeatherType::SNOW) {
			maxParticles = 30000;

			float particlesToSpawn = deltaTime * 150.0f * weatherIntensity;
			spawnAccumulator += particlesToSpawn;

			int newparticles = static_cast<int>(spawnAccumulator);
			spawnAccumulator -= newparticles;

			for (int i = 0; i < newparticles; i++) {
				int unParticles = findUnusedParticle();
				Particle& p = particlesContainer[unParticles];

				float rx = dis(gen);
				float rz = dis(gen);

				p.depth = rz * rz;
				p.pos = glm::vec3(rx * 100.0f, 105.0f, 0.0f);

				float baseFall = -3.0f; 
				float depthFall = -3.0f;

				p.speed.y = baseFall + p.depth * depthFall;
				p.speed.x = ((rand() % 60 - 30) / 10.0f);

				p.life = 15.0f + (rand() % 5); 
				p.size = 0.5f + p.depth * 0.4f;

				p.r = 235; p.g = 238; p.b = 242;
				p.a = 50 + p.depth * 150;
			}
		} else if (currentType == df::types::WeatherType::RAIN) {
			maxParticles = 30000;
			float intensityAbs = std::abs(weatherIntensity);

			float particlesToSpawnFloat = deltaTime * 1000.0f * intensityAbs ;
			spawnAccumulator += particlesToSpawnFloat;

			int newparticles = static_cast<int>(spawnAccumulator);
			spawnAccumulator -= newparticles;

			for (int i = 0; i < newparticles; i++) {
				int unParticles = findUnusedParticle();
				Particle& p = particlesContainer[unParticles];

				float rx = dis(gen);
				float rz = dis(gen);

				p.depth = rz * rz;
				p.pos = glm::vec3(rx * 100.0f, 105.0f, 0.0f);

				float baseFall = -40.0f; 
				float depthFall = -20.0f; 

				p.speed.y = (baseFall + p.depth * depthFall) * (1.0f + intensityAbs * 0.2f);
				p.speed.x = 0.0f;

				p.life = 4.0f + (dis(gen) * 2.0f); 
				p.size = 0.4f + p.depth * 0.15f;

				p.r = 160; p.g = 210; p.b = 255;
				p.a = 140;
			}
		} else if (currentType == df::types::WeatherType::SUNNY){
			maxParticles = 0;	
		}

		particlesCount = 0;
		for (int i = 0; i < maxParticles; i++) {
			Particle& p = particlesContainer[i];
			if (p.life > 0.0f) {
				p.life -= deltaTime;
				p.pos += p.speed * deltaTime;

				if (weatherIntensity > 0.0f) {
					p.pos.x += sin(p.life * 2.0f) * 0.5f * deltaTime;
				}

				if (p.pos.y < -5.0f || p.pos.x < -5.0f || p.pos.x > screenWidth + 5.0f || weatherIntensity==0) {
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

		const glm::mat4 projection = glm::ortho(0.0f, screenWidth, 0.0f, screenHeight, -1.0f, 1.0f);
		render(glm::mat4(1.0f), projection);
	}



	void RenderWeatherSystem::render(const glm::mat4& view, const glm::mat4& projection) noexcept {
		if (particlesCount == 0)
			return;


		// Update buffers
		glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
		glBufferSubData(GL_ARRAY_BUFFER, 0, particlesCount * sizeof(GLfloat) * 4, g_particule_position_size_data.data());

		glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
		glBufferSubData(GL_ARRAY_BUFFER, 0, particlesCount * sizeof(GLubyte) * 4, g_particule_color_data.data());

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		particleShader.use();

		particleShader.setFloat("weatherIntensity", weatherIntensity);

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
