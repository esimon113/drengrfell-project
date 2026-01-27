#pragma once

#include "renderCommon.h"
#include <glm/glm.hpp>
#include <registry.h>
#include <utils/framebuffer.h>
#include <utils/shader.h>
#include <vector>
#include <window.h>

namespace df {

	class RenderWeatherSystem {
	  public:

		static RenderWeatherSystem init(Window* window, Registry* registry, std::shared_ptr<GameState> gamestate) noexcept;
		void increaseIntensity() { weatherIntensity += 0.20f; }
		void decreaseIntensity() { weatherIntensity -= 0.20f; }
		float getIntensity() { return weatherIntensity;}

		void randomizeWeather() noexcept;


		void step(float deltaTime) noexcept;
		void render(const glm::mat4& view, const glm::mat4& projection) noexcept;
		void deinit() noexcept;
		void reset();

	  private:
		Window* window;
		Registry* registry;
		std::shared_ptr<GameState> gameState;

		GLuint billboard_vertex_buffer;
		GLuint particles_position_buffer;
		GLuint particles_color_buffer;
		GLuint vao;

		Shader particleShader;

		int maxParticles;
		int particlesCount;
		df::types::WeatherType currentType;

		float weatherIntensity = 0.0f;

		struct Particle {
			glm::vec3 pos;
			glm::vec3 speed;
			unsigned char r, g, b, a;
			float size;
			float life;
			float depth;
			float cameradistance;
		};

		std::vector<Particle> particlesContainer;

		// CPU buffers that will be sent to GPU
		std::vector<GLfloat> g_particule_position_size_data;
		std::vector<GLubyte> g_particule_color_data;

		// Helper methods
		int findUnusedParticle() noexcept;
		void initBuffers() noexcept;
	};

} // namespace df
