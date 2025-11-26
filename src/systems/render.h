#pragma once

#include <core/types.h>
#include <core/camera.h>

#include <registry.h>
#include <window.h>

#include <utils/mesh.h>
#include <utils/shader.h>
#include <utils/texture.h>
#include <utils/framebuffer.h>

#include "utils/textureArray.h"


namespace df {
	class Player;
	class RenderSystem {
		public:
			RenderSystem() = default;
			~RenderSystem() = default;

			static RenderSystem init(Window* window, Registry* registry) noexcept;
			void deinit() noexcept;

			void step(const float delta) noexcept;
			void reset() noexcept;

			void onResizeCallback(GLFWwindow* window, int width, int height) noexcept;

			void updateFogOfWar(const Player*player) noexcept;

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
			static constexpr int FLOATS_PER_TILE_VERTEX = 4;

			struct TileInstance {
				glm::vec2 position;
				int type;
				int padding;
				int explored; // 0 = unexplored, 1 = explored, maybe 2 for a second player 
			};

			std::vector<float> tileMesh;
			std::vector<TileInstance> tileInstances;

			static std::vector<float> createTileMesh() noexcept;
			static std::vector<float> createRectangularTileMesh() noexcept;
			static std::vector<TileInstance> createTileInstances(int columns = 10.0f, int rows = 10.0f) noexcept;
			static glm::vec3 getTileColor(types::TileType type) noexcept;
			void initMap() noexcept;
			void renderMap(glm::vec2 scale = {1.5f, 1.5f}) const noexcept;
			static glm::vec2 calculateWorldDimensions(int columns = 10.0f, int rows = 10.0f) noexcept;
	};
}
