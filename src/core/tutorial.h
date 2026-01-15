#pragma once
#include <string>

namespace df {

	enum class TutorialStepId {
		WELCOME,
		MOVE_CAMERA,
		CENTER_CAMERA,
		ZOOM_CAMERA,
		MOVE_HERO,
		OPEN_QUEST_MENU,
		BUILD_SETTLEMENT,
		BUILD_ROAD,
		OPEN_KEYBINDS_MENU,
		END
	};

	struct TutorialStep {
		TutorialStepId id;
		std::string text;
		bool completed = false;
		std::optional<glm::vec2> screenPosition;
		bool renderBox = true;
	};

} // namespace df
