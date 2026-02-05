#include "tutorial.h"

namespace df {

	std::vector<TutorialStep> createDefaultTutorial() {
		std::vector<TutorialStep> steps;

		steps.push_back({.id = TutorialStepId::WELCOME,
						 .text =
							 "Welcome to Drengrfell.\n"
							 "You are a lone hero in a harsh land.\n"
							 "This tutorial will help you to overcome the challenges.\n"
							 "You can find the next steps in the top left corner!\n"
							 "Press left mouse button to continue.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::MOVE_CAMERA,
						 .text = "Lets start by looking around.\nUse WASD to move the camera or simply\nmove the cursor to the edges of the window.\nJust try it now!",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::CENTER_CAMERA,
						 .text = "Use 'Space' to center the camera onto the hero.\nThat way you can always find him no matter where you are!",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::ZOOM_CAMERA,
						 .text = "Use the mousewheel to zoom in/out.\nThis is also possible with +/-.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::MOVE_HERO,
						 .text = "Use the right mouse button to click on a tile on the map to select and highlight it.\nAfter pressing the 'End Turn' button on the bottom right the hero will move there.\n"
								 "But beware, you might encounter a hazard.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::BUILD_SETTLEMENT,
						 .text =
							 "Build your first settlement using the 'N' button.\n"
							 "Then you get the hover view.\n"
							 "Here click any free tile close to your hero to build the settlement.\n"
							 "Settlements generate resources from nearby tiles each round.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::BUILD_ROAD,
						 .text = "Build a road to expand using 'B' Button to create the hover view.\nThen select any free edge close to your hero to build the road.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::OPEN_QUEST_MENU,
						 .text = "You can check your quests by pressing 'Q'! Your first quest will be\n to complete the tutorial.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::OPEN_TRADE_MENU,
						 .text = "Use 'T' to open the trade menu and trade your ressources.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::OPEN_COST_MENU,
						 .text = "You can see all the costs by pressing the 'C' button.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::OPEN_KEYBINDS_MENU,
						 .text = "You can see all the possible keybinds by pressing the 'K' button.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::END,
						 .text = "Tutorial completed! \nPress left mouse button to exit the tutorial.",
						 .completed = false,
						 .renderBox = true});

		return steps;
	}

} // namespace df
