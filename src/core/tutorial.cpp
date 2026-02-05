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

		steps.push_back({.id = TutorialStepId::EXPLAIN_HEROPOINTS,
						 .text =
							 "Your goal is to earn a total of 20 hero points.\n"
							 "These are awarded for different actions, which are explained later in this tutorial.\n"
							 "Take on the challenge to earn the needed points in the least rounds possible.\n"
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
						 .text = "Use the right mouse button to click on a tile on the map to select and highlight it.\nAfter pressing the 'End Turn' button on the bottom right\nor ENTER the hero will move there.\n"
								 "But beware, forests, mountains, clay pits and ice are hazardous terrain,\n"
								 "you might be stuck there for a few rounds.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::EXPLAIN_MOVEMENT,
						 .text = "Some terrain is harder to traverse, so your hero\nwill circumvent it if he finds a better way.\n"
								 "Swimming is the hardest, but traversing through fog, mountains, ice or forests\nis hard as well. "
								 "Press left mouse button to continue.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::BUILD_SETTLEMENT,
						 .text =
							 "Build your first settlement using the 'N' button.\n"
							 "Then you get the hover view.\n"
							 "To build the settlement click any free spot where\ntwo tiles meet the one your hero is standing on.\n"
							 "They can not be build right next to each other.\n"
							 "Settlements generate resources from nearby tiles each round.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::BUILD_ROAD,
						 .text = "Build a road to expand using 'B' Button to create the hover view.\nThen select any free edge next to a settlement or road to build the road.\n"
								 "Roads can also be built adjacent to other roads or settlements anywhere on the\nmap, enabling you to also build settlements there.\n"
								 "You can use this to your advantage by creating a road network\nfor easier building, even when far away.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::OPEN_QUEST_MENU,
						 .text = "You can check your quests by pressing 'Q'!\nYour first quest will be to complete the tutorial.\n"
								 "Completing all quests awards you with 5 hero points, so it is worth persuing them!\n"
								 "You can also access the quest menu by clicking the scroll icon on the right.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::OPEN_TRADE_MENU,
						 .text = "Use 'T' to open the trade menu and trade your ressources.\n"
								 "This can be very useful if you are just\na few resources short for building or upgrading.\n"
								 "You can also access the trade menu by clicking the scale icon on the right.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::OPEN_COST_MENU,
						 .text = "You can see the costs of settlements and roads by pressing the 'C' button.\n"
								 "You can also access the cost menu by clicking the coin icon on the right.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::OPEN_KEYBINDS_MENU,
						 .text = "You can see all the possible keybinds by pressing the 'K' button.\n"
								 "You can also access the keybinds menu by clicking the gear icon on the right.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::OPEN_HEROPOINTS_MENU,
						 .text = "You can see which actions award hero points by pressing the 'V' button.\n"
								 "You can also access the hero point menu by\nclicking the laurel wreath icon on the right.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::SETTLEMENT_MENU,
						 .text = "When you click on a settlement you can upgrade it or\nbuild buildings to boost individual recource production.\n"
								 "Upgrading a settlement is also a great source of hero points.",
						 .completed = false,
						 .renderBox = true});

		steps.push_back({.id = TutorialStepId::END,
						 .text = "Tutorial completed! \nPress left mouse button to exit the tutorial.",
						 .completed = false,
						 .renderBox = true});

		return steps;
	}

} // namespace df
