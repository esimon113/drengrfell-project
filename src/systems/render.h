#pragma once

#include <registry.h>
#include <window.h>

#include <utils/mesh.h>
#include <utils/shader.h>
#include <utils/texture.h>
#include <utils/framebuffer.h>

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
		Registry* m_registry;
		Window* m_window;

		Framebuffer m_intermediate_framebuffer;
		Mesh m_chicken_mesh;
		Shader m_chicken_shader;
		Shader m_sprite_shader;
		Shader m_wind_shader;
		Shader m_egg_shader;

		Texture m_eagle_texture;
		Texture m_bug_texture;

		GLuint m_quad_vao;
		GLuint m_quad_ebo;

		struct Egg {
			glm::vec2 m_position;
			float m_radius;
			glm::vec3 m_color;
		};
		GLuint m_egg_vao;
		GLuint m_egg_vbo;

		struct {
			glm::uvec2 m_origin;
			glm::uvec2 m_size;
		} m_viewport;
};
