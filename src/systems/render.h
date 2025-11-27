#pragma once

#include <registry.h>
#include <window.h>

#include <utils/mesh.h>
#include <utils/shader.h>
#include <utils/texture.h>
#include <utils/framebuffer.h>
#include "hero.h"



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
			Texture& getCurrentTexture(AnimationComponent& animComp, int frameIndex);

		private:
			Registry* registry;
			Window* window;

			Framebuffer intermediateFramebuffer;
			Shader spriteShader;
			Shader windShader;
			Shader heroShader;

			GLuint m_quad_vao;
			GLuint m_quad_ebo;

			struct {
				glm::uvec2 m_origin;
				glm::uvec2 m_size;
			} m_viewport;
			std::vector<Texture> heroIdleTextures;
			std::vector<Texture> heroSwimTextures;
			std::vector<Texture> heroAttackTextures;
			std::vector<Texture> heroJumpTextures;
	};
}
