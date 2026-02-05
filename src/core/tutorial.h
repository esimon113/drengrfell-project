#pragma once
#include <string>
#include <vector>

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
		OPEN_TRADE_MENU,
		OPEN_KEYBINDS_MENU,
		OPEN_COST_MENU,
		END
	};

	struct TutorialStep {
		TutorialStepId id;
		std::string text;
		bool completed = false;
		bool renderBox = true;
	};

	std::vector<TutorialStep> createDefaultTutorial();

} // namespace df
