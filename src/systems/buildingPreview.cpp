#include "buildingPreview.h"
#include "core/camera.h"
#include "systems/renderCommon.h"





namespace df {

	BuildingPreviewSystem BuildingPreviewSystem::init(Window* window, Registry* registry, GameState& gameState) noexcept {
		BuildingPreviewSystem self;
		self.window = window;
		self.registry = registry;
		self.gamestate = &gameState;
		self.previewEntity = Entity();
		self.hasPreviewEntity = false;

		return self;
	}


	void BuildingPreviewSystem::deinit() noexcept {
		if (hasPreviewEntity && registry) {
			registry->clear(previewEntity);
			hasPreviewEntity = false;
		}
	}


	void BuildingPreviewSystem::step(float /*dt*/) noexcept {
		if (hasPreviewEntity) {
			updatePreviewPosition();
		}
	}


	void BuildingPreviewSystem::updatePreviewPosition() noexcept {
		if (!hasPreviewEntity || !registry || !window || !gamestate) return;

		Camera& cam = registry->cameras.get(registry->getCamera());
		const Graph& map = gamestate->getMap();

		uint32_t columns = map.getMapWidth();
		uint32_t rows = map.getTileCount() / columns;
		const glm::vec2 worldDimensions = calculateWorldDimensions(columns, rows);

		Viewport viewport;
		viewport.origin = glm::uvec2(0);
		viewport.size = window->getWindowExtent();

		glm::vec2 cursorScreenPos = window->getCursorPosition();
		glm::vec2 cursorWorldOffset = cam.position + screenToWorldCoordinates(
			cursorScreenPos,
			viewport,
			worldDimensions / cam.zoom
		);

		if (registry->positions.has(previewEntity)) {
			registry->positions.get(previewEntity) = cursorWorldOffset;
		} else {
			registry->positions.emplace(previewEntity) = cursorWorldOffset;
		}
	}


	void BuildingPreviewSystem::setSettlementPreviewActive(bool active) noexcept {
		if (!registry) return;

		if (active) {
			if (!hasPreviewEntity) {
				previewEntity = Entity();
				BuildingPreviewComponent& preview = registry->buildingPreviews.emplace(previewEntity);
				preview.type = BuildingPreviewType::Settlement;

				glm::vec2& scale = registry->scales.emplace(previewEntity);
				scale = glm::vec2(0.5f, 0.5f); // TODO: make dynamic?!

				hasPreviewEntity = true;
			} else {
				// Update existing preview type
				if (registry->buildingPreviews.has(previewEntity)) {
					registry->buildingPreviews.get(previewEntity).type = BuildingPreviewType::Settlement;
				}
				if (registry->scales.has(previewEntity)) {
					registry->scales.get(previewEntity) = glm::vec2(0.5f, 0.5f);
				}
			}
		} else {
			// Only clear if this is actually a settlement preview
			if (hasPreviewEntity && registry->buildingPreviews.has(previewEntity)) {
				if (registry->buildingPreviews.get(previewEntity).type == BuildingPreviewType::Settlement) {
					registry->clear(previewEntity);
					hasPreviewEntity = false;
				}
			}
		}
	}


	void BuildingPreviewSystem::setRoadPreviewActive(bool active) noexcept {
		if (!registry) return;

		if (active) {
			if (!hasPreviewEntity) {
				previewEntity = Entity();
				BuildingPreviewComponent& preview = registry->buildingPreviews.emplace(previewEntity);
				preview.type = BuildingPreviewType::Road;

				// Set default scale for road
				glm::vec2& scale = registry->scales.emplace(previewEntity);
				scale = glm::vec2(1.7f, 0.85f); // baseRoadScale * 1.7f

				hasPreviewEntity = true;
			} else {
				// Update existing preview type
				if (registry->buildingPreviews.has(previewEntity)) {
					registry->buildingPreviews.get(previewEntity).type = BuildingPreviewType::Road;
				}
				if (registry->scales.has(previewEntity)) {
					registry->scales.get(previewEntity) = glm::vec2(1.7f, 0.85f);
				}
			}
		} else {
			// Only clear if this is actually a road preview
			if (hasPreviewEntity && registry->buildingPreviews.has(previewEntity)) {
				if (registry->buildingPreviews.get(previewEntity).type == BuildingPreviewType::Road) {
					registry->clear(previewEntity);
					hasPreviewEntity = false;
				}
			}
		}
	}
}
