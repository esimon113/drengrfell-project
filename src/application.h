#pragma once

#include "core/configMenu.h"
#include "core/gamecontroller.h"
#include "core/gamestate.h"
#include "core/mainMenu.h"
#include "worldGeneratorConfig.h"
#include "tradingSystem.h"
#include <common.h>
#include <memory>
#include <utils/commandLineOptions.h>

#include "entityMovement.h"
#include <miniaudio.h>
#include <utils/framebuffer.h>
#include <utils/mesh.h>
#include <utils/shader.h>
#include <utils/texture.h>

#include <systems/buildingPreview.h>
#include <systems/systems.h>
#include "ai/aiSystem.h"

#include <registry.h>
#include <window.h>



namespace df {
	class Application {
	  public:
		// NOTE: You may want to use the constructor and destructor for initialization
		//       and deinitialization of objects. For the template we opted to use explicit
		//       initialization and deinitialization to avoid hidden control flow.
		static ::std::optional<Application> init(const CommandLineOptions& options) noexcept;
		void deinit() noexcept;
		void run() noexcept;
		void toggleMovement() noexcept;

	  private:
		std::unique_ptr<Window> window;
		Window* debugWindow = nullptr;
		Registry* registry;

		std::shared_ptr<EventBus> eventBus;
		std::unique_ptr<AudioSystem> audioEngine;
		std::unique_ptr<AiSystem> aiSystem;

		WorldSystem world;
		// PhysicsSystem physics;

		RenderSystem render;

		std::unique_ptr<EntityMovementSystem> movementSystem;
		BuildingPreviewSystem buildingPreviewSystem;
		TradingSystem tradingSystem;
		EventPresentationSystem eventPresentationSystem;

		void reset() noexcept;

		void startGame(int seed, int width, int height, int mode) noexcept;
		void configurateGame() noexcept;
		void setInsular() noexcept;
		void setPerlin() noexcept;
		void generateMap(WorldGeneratorConfig config) noexcept;

		void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;
		void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) noexcept;
		void onScrollCallback(GLFWwindow* window, double xoffset, double yoffset) noexcept;
		void onResizeCallback(GLFWwindow* window, int width, int height) noexcept;
		void spawnHero() noexcept;


		bool test = false;

		bool victoryScreenClosed = false;
		bool victoryScreenShown = false;
		bool awaitingTurnEnd = false;
		size_t selectedSettlementId = SIZE_MAX;

		// GameState
		std::shared_ptr<GameState> gameState;
		// GameController
		std::shared_ptr<GameController> gameController;
		// MainMenu
		MainMenu mainMenu;
		// ConfigMenu
		ConfigMenu configMenu;

		// TODO: adjust for multiple players + ending game + reentering
		types::GamePhase previousGamePhase = types::GamePhase::START;
	};
} // namespace df
