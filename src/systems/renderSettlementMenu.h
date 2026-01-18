#pragma once

#include "gamestate.h"
#include "gamecontroller.h"
#include "registry.h"
#include "renderCommon.h"
#include "window.h"
#include <renderText.h>
#include <utils/shader.h>

namespace df {

	class RenderSettlementMenuSystem {
	  public:
		RenderSettlementMenuSystem() = default;
		~RenderSettlementMenuSystem() = default;

		static RenderSettlementMenuSystem init(Window* window, Registry* registry, std::shared_ptr<GameState> gameState, GameController* gameController) noexcept;
		void deinit() noexcept;
		void reset() noexcept;
		void step(float dt) noexcept;

		void showMenu(size_t settlementId);
		void close() noexcept;
		bool isActive() const noexcept { return active; }
		bool isPointInsideMenu(glm::vec2 mouse) const noexcept;

		// Returns true if the click was handled
		bool onMouseButton(glm::vec2 mouse, int button, int action) noexcept;
		void onKeyCallback(int key, int action) noexcept;
		void onResizeCallback(GLFWwindow*, int width, int height) noexcept;
		void updateViewport(const glm::uvec2& origin, const glm::uvec2& size) noexcept {
			viewport.origin = origin;
			viewport.size = size;
			if (active) {
				rebuildLayout();
			}
		}

	  private:
		enum class ButtonAction {
			UpgradeStone,
			UpgradeCastle,
			BuildProductivity,
			Close
		};

		struct Button {
			std::string text;
			ButtonAction action;
			types::TileType tileType{types::TileType::EMPTY};
			size_t tileId{SIZE_MAX};
			float x{}, y{}, w{}, h{};
		};

		void rebuildLayout();
		void renderBox(glm::vec2 pos, glm::vec2 size, glm::vec3 color) const noexcept;
		std::string formatCostLine(const std::vector<int>& cost) const;
		int getPotencyPercent(types::TilePotency potency) const;

		Window* window = nullptr;
		Registry* registry = nullptr;
		std::shared_ptr<GameState> gameState;
		GameController* gameController = nullptr;
		Viewport viewport;

		Shader rectShader;
		GLuint quadVao = 0;
		GLuint vbo = 0;

		bool active = false;
		size_t selectedSettlementId = SIZE_MAX;
		std::string title;
		std::vector<std::string> textLines;
		std::vector<std::string> displayLines;
		std::vector<glm::vec2> textPositions;
		std::vector<Button> buttons;

		glm::vec2 boxPos{};
		glm::vec2 boxSize{};
		float separatorY = -1.0f;
		float scale = 1.0f;
		float paddingX = 0.0f;
		float paddingY = 0.0f;

		float DEFAULT_WIDTH = 1920.0f;
		float DEFAULT_HEIGHT = 1080.0f;
	};

} // namespace df

