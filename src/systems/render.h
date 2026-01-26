#pragma once

#include "renderCommon.h"

#include <registry.h>
#include <utils/framebuffer.h>
#include <utils/shader.h>
#include <window.h>


#include "renderBuildingPreviews.h"
#include "renderBuildings.h"
#include "renderHero.h"
#include "renderHud.h"
#include "renderWeather.h"
#include "renderText.h"
#include "renderTiles.h"
#include "renderNotification.h"
#include "eventPresentation.h"
#include "events/eventBus.h"
#include "renderSettlementMenu.h"

namespace df {
	class GameController;

	class RenderSystem {
	  public:
		RenderSystem() = default;
		~RenderSystem() = default;

		static RenderSystem init(Window* window, Registry* registry, std::shared_ptr<GameState> gameState, GameController* gameController, EventBus* eventBus) noexcept;
		void deinit() noexcept;

		void step(float dt) noexcept;
		void reset() noexcept;

		void onResizeCallback(GLFWwindow* window, int width, int height) noexcept;
		void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;
		void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) noexcept;

			RenderTilesSystem renderTilesSystem;
			RenderHeroSystem renderHeroSystem;
			RenderBuildingsSystem renderBuildingsSystem;
			RenderBuildingPreviewsSystem renderBuildingPreviewsSystem;
			RenderTextSystem renderTextSystem;
			RenderHudSystem renderHudSystem;
			RenderWeatherSystem renderWeatherSystem;
			RenderNotificationSystem renderNotificationSystem;
			EventPresentationSystem eventPresentationSystem;
			RenderSettlementMenuSystem renderSettlementMenuSystem;

			RenderTextSystem& getRenderTextSystem() noexcept {
				return renderTextSystem;
			}
			RenderNotificationSystem& getRenderNotificationSystem() noexcept {
				return renderNotificationSystem;
			}
			RenderWeatherSystem& getRenderWeatherSystem() noexcept {
				return renderWeatherSystem;
			}
			RenderTilesSystem& getRenderTilesSystem() noexcept {
				return renderTilesSystem;
			}
			EventPresentationSystem& getEventPresentationSystem() noexcept {
				return eventPresentationSystem;
			}

	  private:
		Registry* registry;
		Window* window;

		Viewport viewport = Viewport();
		Framebuffer intermediateFramebuffer;
	};
} // namespace df
