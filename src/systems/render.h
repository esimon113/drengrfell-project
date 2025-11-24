#pragma once

#include <core/types.h>

#include <registry.h>
#include <window.h>

#include <utils/mesh.h>
#include <utils/shader.h>
#include <utils/texture.h>
#include <utils/framebuffer.h>

#include "utils/textureArray.h"


namespace df {
	class RenderSystem {
		public:
			RenderSystem() = default;
			~RenderSystem() = default;

			static RenderSystem init(Window* window, Registry* registry) noexcept;
			void deinit() noexcept;

			void step(const float delta) noexcept;
			void reset() noexcept;

			void onResizeCallback(GLFWwindow* window, int width, int height) noexcept;

		private:
			Registry* registry;
			Window* window;

			Framebuffer intermediateFramebuffer;
			Shader spriteShader;
			Shader windShader;
			Shader tileShader;

			GLuint m_quad_vao;
			GLuint m_quad_ebo;

			GLuint tileVao;
			GLuint tileVbo;
			GLuint tileInstanceVbo;
			TextureArray tileAtlas;

			struct {
				glm::uvec2 m_origin;
				glm::uvec2 m_size;
			} m_viewport;

			struct TileVertex {
				glm::vec2 position;
				glm::vec2 uv;
			};
			struct TileInstance {
				glm::vec2 position;
				int type;
				int padding;
			};

			std::vector<float> tileMesh;
			std::vector<TileInstance> tileInstances;

			static std::vector<float> createTileMesh(float tileScale = 10.0f) noexcept;
			static std::vector<TileInstance> createTileInstances(int columns = 10.0f, int rows = 10.0f, float tileScale = 10.0f) noexcept;
			static glm::vec3 getTileColor(types::TileType type) noexcept;
			void initMap() noexcept;
			void renderMap() const noexcept;
			static glm::vec2 calculateWorldDimensions(int columns = 10.0f, int rows = 10.0f, float tileScale = 10.0f) noexcept;
	};
}
