#pragma once

#include <core/camera.h>
#include <registry.h>
#include <window.h>
#include <utils/shader.h>
#include <utils/texture.h>

#include "framebuffer.h"

namespace df {
	class RenderBuildingsSystem {
		public:
			RenderBuildingsSystem() = default;
			~RenderBuildingsSystem() = default;

			static RenderBuildingsSystem init(Window* window, Registry* registry) noexcept;
			void deinit() noexcept;
			void step(float dt) noexcept;
			void reset() noexcept;

			void renderSettlementPreview(const glm::vec2& worldPosition, bool active, float time = 0.0f) noexcept;
			void renderRoadPreview(const glm::vec2& worldPosition, bool active, float time = 0.0f) noexcept;

			glm::vec2 screenToWorldCoordinates(const glm::vec2& screenPos) const noexcept;
		private:
			Registry* registry;
			Window* window;

			Framebuffer intermediateFramebuffer;

			Shader buildingHoverShader;
			Shader buildingShadowShader;
			Texture settlementTexture;
			Texture roadPreviewTexture;

			GLuint m_quad_vao;
			GLuint m_quad_ebo;

			struct {
				glm::uvec2 m_origin;
				glm::uvec2 m_size;
			} m_viewport;

			static glm::vec2 calculateWorldDimensions(int columns = 10.0f, int rows = 10.0f) noexcept;
	};
}
