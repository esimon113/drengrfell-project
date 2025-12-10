#pragma once

#include "renderCommon.h"
#include <registry.h>
#include <window.h>
#include <utils/shader.h>
#include <utils/texture.h>
#include <utils/framebuffer.h>


namespace df {
    class RenderHeroSystem {
		public:
			RenderHeroSystem() = default;
			~RenderHeroSystem() = default;

			static RenderHeroSystem init(Window* window, Registry* registry) noexcept;
			void deinit() noexcept;

			void step(float delta) noexcept;
			void reset() noexcept;

			Texture& getCurrentTexture(AnimationComponent& animComp, int frameIndex);
		private:
			Registry* registry;
			Window* window;

			Framebuffer intermediateFramebuffer;
			Shader heroShader;

			GLuint m_quad_vao;
			GLuint m_quad_ebo;

			Viewport viewport;

			std::vector<Texture> heroIdleTextures;
			std::vector<Texture> heroSwimTextures;
			std::vector<Texture> heroAttackTextures;
			std::vector<Texture> heroJumpTextures;
	};
}
