#pragma once

#include "registry.h"
#include "window.h"
#include "gamestate.h"





namespace df {
	class BuildingPreviewSystem {
	public:
		BuildingPreviewSystem() = default;
		~BuildingPreviewSystem() = default;

		static BuildingPreviewSystem init(Window* window, Registry* registry, GameState& gameState) noexcept;
		void deinit() noexcept;
		void step(float dt) noexcept;

		// Update preview entity position based on cursor position
		void updatePreviewPosition() noexcept;

		// Create or update the preview entity for settlements
		void setSettlementPreviewActive(bool active) noexcept;

		// Create or update preview entity for roads
		void setRoadPreviewActive(bool active) noexcept;

		// Get the current preview entity
		Entity getPreviewEntity() const noexcept { return previewEntity; }


	private:
		Registry* registry = nullptr;
		Window* window = nullptr;
		GameState* gamestate = nullptr;
		Entity previewEntity;
		bool hasPreviewEntity = false;
	};
}
