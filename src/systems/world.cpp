#include "world.h"
#include "fmt/base.h"
#include "hero.h"
#include "questsSystem.h"
#include "renderNotification.h"
#include <iostream>

namespace df {
	WorldSystem WorldSystem::init(Window* window, Registry* registry, AudioSystem* audioEngine, GameState& gameState) noexcept {
		WorldSystem self;

		self.window = window;
		self.registry = registry;
		self.audioEngine = audioEngine;
		self.gameState = &gameState;
		self.score = 0;

		self.randomEngine = std::default_random_engine(std::random_device()());

		self.m_reset = true;
		std::cout << "[Debug] WorldSystem::init aufgerufen\n";


		return self;
	}


	void WorldSystem::deinit() noexcept {}


	void WorldSystem::reset() noexcept {
		score = 0;
		m_reset = false;
	}


	void WorldSystem::step(const float delta) noexcept {
		// std::string title = fmt::format("Score: {} - FPS: {:.2f} ({:.2f} ms)", score, 1/delta, 1000 * delta);
		// window->setTitle(title.c_str());

		Camera& cam = registry->cameras.get(registry->getCamera());
		cam.updateView(window->getWindowExtent());
		CameraInput& input = registry->cameraInputs.get(registry->getCamera());

		// each settlement is one point so we update score -> once multiplayer
		// Player& player = registry->players.get(registry->getPlayer());
		// score = player.getSettlementIds().size();

		int fbWidth, fbHeight;
		glfwGetFramebufferSize(window->getHandle(), &fbWidth, &fbHeight);
		auto [scaledMouseX, scaledMouseY] = calculateScaledMousePosition();

		double edgePercent = 0.03;	// How big the zone is where the camera moves on the window edge
		double edgeX = fbWidth * edgePercent;
		double edgeY = fbHeight * edgePercent;


		const Graph& map = gameState->getMap();
		int mapWidth = map.getMapWidth();
		int mapHeight = map.getTileCount() / mapWidth;
		float worldWidth = 2.0f * mapWidth;
		float worldHeight = (mapHeight - 1) * 1.5f + 1.0f;

		float offset = cam.camOffset;
		float camMinX = 0.0f - offset;
		float camMinY = 0.0f - offset;
		float camMaxX = worldWidth - cam.viewWidth + offset / 2;
		float camMaxY = worldHeight - cam.viewHeight + offset;

		if (input.up || scaledMouseY < edgeY) {
			cam.position.y += cam.scrollSpeed * delta;
			completeCameraTutorial();
		}
		if (input.down || scaledMouseY > fbHeight - edgeY) {
			cam.position.y -= cam.scrollSpeed * delta;
			completeCameraTutorial();
		}
		if (input.left || scaledMouseX < edgeX) {
			cam.position.x -= cam.scrollSpeed * delta;
			completeCameraTutorial();
		}
		if (input.right || scaledMouseX > fbWidth - edgeX) {
			cam.position.x += cam.scrollSpeed * delta;
			completeCameraTutorial();
		}
		if (cam.position.x > camMaxX)
			cam.position.x = camMaxX;
		if (cam.position.y > camMaxY)
			cam.position.y = camMaxY;
		if (cam.position.x < camMinX)
			cam.position.x = camMinX;
		if (cam.position.y < camMinY)
			cam.position.y = camMinY;
	}

	void WorldSystem::completeCameraTutorial() {
		auto* step = this->gameState->getCurrentTutorialStep();
		// Update Tutorial if step == moveCamera
		if (step && step->id == TutorialStepId::MOVE_CAMERA) {
			this->gameState->completeCurrentTutorialStep();
		}
	}

	void WorldSystem::centerCameraOnPoint(glm::vec2 pos) {
		Camera& cam = registry->cameras.get(registry->getCamera());
		cam.position.x = pos.x - (0.5f * cam.viewWidth);
		cam.position.y = pos.y - (0.5f * cam.viewHeight);
	}


	void WorldSystem::onKeyCallback(GLFWwindow* /* window */, int key, int /* scancode */, int action, int /* mods */) noexcept {
		CameraInput& input = registry->cameraInputs.get(registry->getCamera());
		Entity hero = registry->animations.entities.front();
		auto& animComp = registry->animations.get(hero);
		auto* step = this->gameState->getCurrentTutorialStep();

		if (this->gameState->isGameOver()) {
			return; 
		}
		switch (action) {
		case GLFW_PRESS:
			switch (key) {

				// ----------------------currently only here for testing until we have a triggerpoint--------------------------------------
			case GLFW_KEY_F7:
				animComp.currentType = Hero::AnimationType::Idle;
				animComp.anim.setCurrentFrameIndex(0);
				fmt::println("Debug: Idle animation activated");
				break;
			case GLFW_KEY_F8:
				animComp.currentType = Hero::AnimationType::Swim;
				animComp.anim.setCurrentFrameIndex(0);
				fmt::println("Debug: Swim animation activated");
				break;
			case GLFW_KEY_F9:
				animComp.currentType = Hero::AnimationType::Attack;
				animComp.anim.setCurrentFrameIndex(0);
				fmt::println("Debug: Attack animation activated");
				break;

			case GLFW_KEY_F10:
				animComp.currentType = Hero::AnimationType::Jump;
				animComp.anim.setCurrentFrameIndex(0);
				fmt::println("Debug: Jump animation activated");
				break;
			case GLFW_KEY_F11:

				animComp.currentType = Hero::AnimationType::Run;
				animComp.anim.setCurrentFrameIndex(0);
				fmt::println("Debug: Run animation activated");
				break;
			case GLFW_KEY_H:
				heroMovementState = !heroMovementState;
				fmt::println("Hero movement mode toggled: {}", heroMovementState ? "ON" : "OFF");
				break;
				// ------------------------------------------------------------

			case GLFW_KEY_R: // pressing the 'r' key triggers a reset of the game
				m_reset = true;
				break;
			case GLFW_KEY_W:
				fmt::println("W pressed");
				input.up = true;
				break;
			case GLFW_KEY_A:
				fmt::println("A pressed");
				input.left = true;
				break;
			case GLFW_KEY_S:
				fmt::println("S pressed");
				input.down = true;
				break;
			case GLFW_KEY_D:
				fmt::println("D pressed");
				input.right = true;
				break;
			case GLFW_KEY_N:
				this->isSettlementPreviewActive = !this->isSettlementPreviewActive;
				fmt::println("Toggled Settlement Preview: {}", this->isSettlementPreviewActive);
				if (this->isSettlementPreviewActive) {
					this->isRoadPreviewActive = false;
				}
				break;
			case GLFW_KEY_B:
				this->isRoadPreviewActive = !this->isRoadPreviewActive;
				fmt::println("Toggled Road Preview: {}", this->isRoadPreviewActive);
				if (this->isRoadPreviewActive) {
					this->isSettlementPreviewActive = false;
				}
				break;
			case GLFW_KEY_Q: {
				showTrade = false;
				auto* quests = registry->getSystem<QuestsSystem>();
				if (quests) {
					quests->notifyNextActiveQuest(); 
				}
				if (step && step->id == TutorialStepId::OPEN_QUEST_MENU) {
					this->gameState->completeCurrentTutorialStep();
				}
			} 
				break;
			case GLFW_KEY_T: {
				auto* notifications = registry->getSystem<RenderNotificationSystem>();
				if(showTrade){
					showTrade = false;
					notifications->close();
				} else {
					showTrade = true;
					if (tradeCallback) {
						tradeCallback();
					}
					if (step && step->id == TutorialStepId::OPEN_TRADE_MENU) {
						this->gameState->completeCurrentTutorialStep();
					}
				
				}
				
				
			} break;
			case GLFW_KEY_K:{
				showTrade = false;
				auto* notifications = registry->getSystem<RenderNotificationSystem>();
				std::vector<std::string> buttons;
				std::string keybindsList = 
					"WASD: Move map\n"
					"Q: Active quests\n"
					"N: Build settlement\n"
					"B: Build road\n"
					"T: Open trade menu\n"
					"C: See costs\n"
					"+/-: Zoom\n"
					"Space: Center camera to hero";
				buttons = {"Close"};
				notifications->showNotification("Keybinds", keybindsList, buttons);
				if (step && step->id == TutorialStepId::OPEN_KEYBINDS_MENU) {
					this->gameState->completeCurrentTutorialStep();
				}
			}
				break;
			
			case GLFW_KEY_C:{
				showTrade = false;
				auto* notifications = registry->getSystem<RenderNotificationSystem>();
				std::vector<std::string> buttons;
				std::string costsList = 
					"SETTLEMENT\n"
					"  - 5 wood\n"
					"  - 5 clay\n"
					"  - 3 grain\n"
					"  - 3 wool\n"
					"ROAD\n"
					"  - 1 wood\n"
					"  - 1 clay";
				buttons = {"Close"};
				notifications->showNotification("COSTS", costsList, buttons);
			}
				break;
			case GLFW_KEY_G: {
				Graph& map = this->gameState->getMap();
				if (const auto worldGenConfResult = WorldGeneratorConfig::deserialize(); worldGenConfResult.isErr()) {
					std::cerr << worldGenConfResult.unwrapErr() << std::endl;
					break;
				} else {
					map.regenerate(worldGenConfResult.unwrap<>());
				}

				if (Player* player = this->gameState->getPlayer(0)) {
					const int width = map.getMapWidth();
					const int height = map.getTileCount() / width;

					player->forgetExploredTiles();
					for (int row = 0; row < height; ++row) {
						for (int col = 0; col < width; ++col) {
							if (uniformDistribution(randomEngine) > 0.25f) {
								player->exploreTile(row * width + col);
							}
						}
					}
				}
			} break;
			case GLFW_KEY_RIGHT_BRACKET:
				// This case is the key which can produce +, *, ~ on the german keyboard layout, so a plus
				calcNewCameraZoom(1.0f);
				// if current step is ZOOM_CAMERA -> complete step
				if (step && step->id == TutorialStepId::ZOOM_CAMERA) {
					this->gameState->completeCurrentTutorialStep();
				}
				break;
			case GLFW_KEY_SLASH:
				// This case is the key which can produce -, _ on the german keyboard layout, so a minus
				calcNewCameraZoom(-1.0f);
				// if current step is ZOOM_CAMERA -> complete step
				if (step && step->id == TutorialStepId::ZOOM_CAMERA) {
					this->gameState->completeCurrentTutorialStep();
				}
				break;
			case GLFW_KEY_KP_ADD:
				// This case is the + key on the numpad
				calcNewCameraZoom(1.0f);
				// if current step is ZOOM_CAMERA -> complete step
				if (step && step->id == TutorialStepId::ZOOM_CAMERA) {
					this->gameState->completeCurrentTutorialStep();
				}
				break;
			case GLFW_KEY_KP_SUBTRACT:
				// This case is the - key on the numpad
				calcNewCameraZoom(-1.0f);
				// if current step is ZOOM_CAMERA -> complete step
				if (step && step->id == TutorialStepId::ZOOM_CAMERA) {
					this->gameState->completeCurrentTutorialStep();
				}
				break;
			
			case GLFW_KEY_SPACE: {
				// TODO: for multiplayer get the hero of the current player
				Entity e = registry->animations.entities.front();
				auto pos = registry->positions.get(e);
				centerCameraOnPoint(pos);
				if (step && step->id == TutorialStepId::CENTER_CAMERA) {
					this->gameState->completeCurrentTutorialStep();
				}
			}
				break;
			default:
				break;
			}
			break;

		case GLFW_RELEASE:
			switch (key) {
			case GLFW_KEY_W:
				input.up = false;
				break;
			case GLFW_KEY_A:
				input.left = false;
				break;
			case GLFW_KEY_S:
				input.down = false;
				break;
			case GLFW_KEY_D:
				input.right = false;
				break;
			}
			break;

		case GLFW_REPEAT:
		default:
			break;
		}
	}

	double WorldSystem::getMouseX() {
		return mouseX;
	}
	double WorldSystem::getMouseY() {
		return mouseY;
	}

	std::pair<double, double> WorldSystem::calculateScaledMousePosition() {
		double rawX, rawY;
		glfwGetCursorPos(this->window->getHandle(), &rawX, &rawY);

		int winWidth, winHeight;
		glfwGetWindowSize(this->window->getHandle(), &winWidth, &winHeight);

		int fbWidth, fbHeight;
		glfwGetFramebufferSize(this->window->getHandle(), &fbWidth, &fbHeight);

		float xScale = (winWidth > 0) ? (float)fbWidth / winWidth : 1.f;
		float yScale = (winHeight > 0) ? (float)fbHeight / winHeight : 1.f;

		return {rawX * xScale, rawY * yScale};
	}

	void WorldSystem::onMouseButtonCallback(GLFWwindow* /* windowParam */, int button, int action, int /* mods */) noexcept {
		auto* step = this->gameState->getCurrentTutorialStep();

		// Changed how mouse is captured

		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			// LMB gedrückt

			//auto [ scaledMouseX, scaledMouseY ] = calculateScaledMousePosition();
			
			//mouseX = scaledMouseX;
			//mouseY = scaledMouseY;
			
			//fmt::println("LMB pressed at screen coordinates: ({}, {})", mouseX, mouseY);

			// Update Tutorial if finished
			if (step && step->id == TutorialStepId::WELCOME) {
				this->gameState->completeCurrentTutorialStep();
			} else if (step && step->id == TutorialStepId::END) {
				this->gameState->completeCurrentTutorialStep();
				auto* quests = registry->getSystem<QuestsSystem>();
				if (quests) {
					quests->updateProgress(types::QuestGoalType::TUTORIAL, 1);
				}
			}
		} else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
			// RMB gedrückt
			auto [scaledMouseX, scaledMouseY] = calculateScaledMousePosition();

			mouseX = scaledMouseX;
			mouseY = scaledMouseY;
			
			fmt::println("RMB pressed at screen coordinates: ({}, {})", mouseX, mouseY);
		}
	}

	void WorldSystem::onScrollCallback(GLFWwindow*, double /* xoffset */, double yoffset) noexcept {
		fmt::println("Scrolled: {}", yoffset);
		calcNewCameraZoom(yoffset);
		// if current step is ZOOM_CAMERA -> complete step
		auto* step = this->gameState->getCurrentTutorialStep();
		if (step && step->id == TutorialStepId::ZOOM_CAMERA) {
			this->gameState->completeCurrentTutorialStep();
		}
	}

	void WorldSystem::calcNewCameraZoom(double yoffset) noexcept {
		Camera& cam = registry->cameras.get(registry->getCamera());
		// linear zoom means the effect gets greater the farther we zoom out and smaller the more we zoom in
		// we may want to opt for zoom depending on the current zoom factor, depending on our preference and which feels better
		cam.zoom += yoffset * 0.1f;
		if (cam.zoom > cam.zoomMaxValue)
			cam.zoom = cam.zoomMaxValue;
		if (cam.zoom < cam.zoomMinValue)
			cam.zoom = cam.zoomMinValue;
		fmt::println("Zoom now: {}", cam.zoom);
	}


} // namespace df
