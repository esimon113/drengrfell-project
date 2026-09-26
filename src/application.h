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
#include <optional>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "multiplayer/midgard.h"



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
		std::shared_ptr<AiSystem> aiSystem;

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
		void presentHazardState(size_t playerId, bool hadHazardBefore, const std::string& previousHazardName) noexcept;
		void enqueueNet(std::function<void()> fn) noexcept;
		void drainNet() noexcept;
		void onLobby(const df::bifrost::LobbyState& lobby) noexcept;
		void onAuthoritativeState(const nlohmann::json& state) noexcept;
		void onActionResult(bool success, const std::optional<df::bifrost::ErrorInfo>& error) noexcept;
		void requestEndTurn() noexcept;
		void placeHeroFromServer(bool force) noexcept;


		bool test = false;

		bool victoryScreenClosed = false;
		bool victoryScreenShown = false;
		size_t selectedSettlementId = SIZE_MAX;

		std::unique_ptr<df::bifrost::Midgard> midgard;
		std::unique_ptr<std::mutex> netMutex;
		std::vector<std::function<void()>> netQueue;
		std::string serverHost{"127.0.0.1"};
		uint16_t serverPort{7777};
		std::string playerName{"Player"};
		int pendingSeed{-1};
		int pendingWidth{-1};
		int pendingHeight{-1};
		int pendingMode{-1};
		bool configSent{false};
		bool readySent{false};
		bool startSent{false};
		bool joining{false};
		bool soloRequested{false};
		bool sessionMapReady{false};
		bool heroPlaced{false};
		bool tradingReady{false};
		bool hazardPresentationPending{false};
		bool hazardHadBefore{false};
		std::string hazardPreviousName;
		size_t hazardPlayerId{0};

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
