#pragma once

#include "registry.h"
#include "renderCommon.h"
#include "window.h"
#include <renderText.h>
#include <utils/shader.h>
#include "utils/texture.h"

namespace df {

	class RenderNotificationSystem {
	  public:
		RenderNotificationSystem() = default;

		static RenderNotificationSystem init(Window* window, Registry* registry) noexcept;
		void deinit() noexcept;
		void reset() noexcept;
		void step(float dt) noexcept;

		void showNotification(const std::string& title, const std::string& message, const std::vector<std::string>& buttonTexts);

		// Returns pressed button text or empty string
		std::string onMouseButton(glm::vec2 mouse, int button, int action) noexcept;
		bool isActive() const noexcept { return active; }
		void close() noexcept { active = false; }
		void onResizeCallback(GLFWwindow*, int width, int height) noexcept;
		void renderBox(glm::vec2 pos, glm::vec2 size, glm::vec3 color) const noexcept;

		void drawSprite(Texture& tex, glm::vec2 pos, glm::vec2 size, glm::vec3 color) const noexcept;

		void updateViewport(const glm::uvec2& origin, const glm::uvec2& size) noexcept {
			viewport.origin = origin;
			viewport.size = size;
		}

	  private:
		struct Button {
			std::string text;
			float x{}, y{}, w{}, h{};
		};

		Window* window = nullptr;
		Registry* registry = nullptr;
		Viewport viewport;

		Shader rectShader;
		GLuint quadVao = 0;
		GLuint vbo = 0;

		// Texture for Background/Buttons
		Shader textureShader;
		Texture notificationBackgroundTexture;
		Texture notificationButtonTexture;

		std::string title;
		std::string message;
		std::vector<Button> buttons;
		std::vector<std::string> buttonTexts;
		bool active = false;

		glm::vec2 boxPos{};
		glm::vec2 boxSize{};
		float scale = 1.0f;
		float paddingX = 0.0f;
		float paddingY = 0.0f;

		float DEFAULT_WIDTH = 1920.0f;
		float DEFAULT_HEIGHT = 1080.0f;
	};

} // namespace df
