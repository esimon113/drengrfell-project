#pragma once
#include "registry.h"
#include "renderCommon.h"
#include "window.h"
#include <renderText.h>
#include <eventPresentation.h>
#include <renderSettlementMenu.h>
#include <utils/shader.h>
#include "utils/texture.h"

namespace df {
	class RenderHudSystem {
	  public:
		RenderHudSystem() = default;
		~RenderHudSystem() = default;

		struct SideHudButton {
			float x, y, w, h;
			std::string label;
			Texture icon;
		};

		static RenderHudSystem init(Window* window, Registry* registry, std::shared_ptr<GameState> gameState) noexcept;
		void scaleHud() noexcept;
		void deinit() noexcept;
		void step(float dt) noexcept;
		bool isMouseOverSideHudButton(const SideHudButton& btn, glm::vec2 mouse) const noexcept;
		std::string getSideHudButtonClicked(glm::vec2 mouse, int button, int action) const noexcept;
		void reset() noexcept;

		void renderRectBox(glm::vec2 pos, glm::vec2 size, glm::vec3 color) const noexcept;
		void drawSprite(Texture& tex, glm::vec2 pos, glm::vec2 size, glm::vec3 color) const noexcept;
		bool isMouseOverEndTurn(glm::vec2 mouse) const noexcept;
		bool onMouseButton(glm::vec2 mouse, int button, int action) noexcept;
		bool wasEndTurnClicked(glm::vec2 mouse, int button, int action) const noexcept;
		void onResizeCallback(GLFWwindow*, int width, int height) noexcept;
		void updateViewport(const glm::uvec2& origin, const glm::uvec2& size) noexcept {
			this->viewport.origin = origin;
			this->viewport.size = size;
		}
		std::string getLastSideHudButtonPressed() {
			return lastSideHudButtonPressed;
		}

		void setHudColors(std::vector<glm::vec3> colors) {
			hudResourceColors = colors;
		}

	  private:
		Registry* registry = nullptr;
		Window* window = nullptr;
		std::shared_ptr<GameState> gameState;
		Viewport viewport;

		// bottom hud background
		Texture hudBackgroundTexture;
		Texture tutorialTexture;

		Texture hudEndTurnButtonTexture;
		// ressources textures
		Texture woodTexture;
		Texture stoneTexture;
		Texture clayTexture;
		Texture woolTexture;
		Texture grainTexture;
		Texture heroPointsTexture;
		std::vector<std::tuple<Texture, int, glm::vec3>> resourceIconsWithAmount;

		// shader
		Shader rectShader;
		Shader textureShader;
		GLuint quadVao = 0;
		GLuint vbo = 0;
		glm::vec2 hudSize;
		glm::vec2 hudPos;
		glm::vec2 iconPos;
		float iconSize;
		float iconPadding;

		struct Button {
			// rectangle in pixels; origin is bottom-left
			float x, y, w, h;
		};

		Button endTurnButton{};

		// Side hud for ui interactions with trade, quest, ...
		glm::vec2 sideHudPos;
		glm::vec2 sideHudSize;

		std::vector<SideHudButton> sideButtons;

		Texture tradeTexture;
		Texture questTexture;
		Texture keybindingsTexture;
		Texture costTexture;
		Texture heroPointsSideHudTexture;

		// last button pressed on the side hud for ui interactions
		std::string lastSideHudButtonPressed = "";
		std::vector<glm::vec3> hudResourceColors = {{1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}};

		float DEFAULT_WIDTH = 1920.0f;
		float DEFAULT_HEIGHT = 1080.0f;
	};
} // namespace df
