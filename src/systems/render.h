#pragma once

#include <core/types.h>

#include <registry.h>
#include <window.h>

#include <utils/mesh.h>
#include <utils/shader.h>
#include <utils/texture.h>
#include <utils/framebuffer.h>



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

			GLuint m_quad_vao;
			GLuint m_quad_ebo;

			struct {
				glm::uvec2 m_origin;
				glm::uvec2 m_size;
			} m_viewport;

			struct TileInstance {
				glm::vec2 position;
				int type;
			};

			static std::vector<float> createTileMesh(float tileScale) noexcept;
			static std::vector<TileInstance> createTileInstances(int columns, int rows, float tileScale) noexcept;
			static glm::vec3 getTileColor(types::TileType type) noexcept;
	};
}
