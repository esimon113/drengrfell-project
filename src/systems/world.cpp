#include "world.h"
#include "hero.h"
#include <iostream>
#include "fmt/base.h"
#


namespace df {
	WorldSystem WorldSystem::init(Window* window, Registry *registry, AudioSystem *audioEngine, GameState& gameState) noexcept {
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
		//std::string title = fmt::format("Score: {} - FPS: {:.2f} ({:.2f} ms)", score, 1/delta, 1000 * delta);
		//window->setTitle(title.c_str());

		Camera& cam = registry->cameras.get(registry->getCamera());
		CameraInput& input = registry->cameraInputs.get(registry->getCamera());

		//each settlement is one point so we update score -> once multiplayer 
		//Player& player = registry->players.get(registry->getPlayer()); 
		//score = player.getSettlementIds().size();

		if(score>=10){
			fmt::println("End of the game, you win!");
			// Implement proper game ending logic here -> close the window for now
			window->close();
		}

		// The world min and max values would need to be set dynamically depending on the world dimensions, once we save that outside the render.cpp
		// these values are just placeholders which work well for now, but are determined by testing alone
		float worldXMin = 0.0f;
		float worldYMin = 0.0f;
		float worldXMax = 3.5f;
		float worldYMax = 3.0f;
		float offset = cam.camOffset;
		float camMinX = worldXMin - offset;
		float camMinY = worldYMin - offset;
		// the offset is scaled by the zoom so the map is not cut off at high zoom and not too much blank space is visible at low zoom
		float camMaxX = worldXMax + offset * cam.zoom;
		float camMaxY = worldYMax + offset * cam.zoom;


		// scaling with cam.zoom makes the camera move faster when zoomed in and slower when zoomed out
		// we may want to test what scaling feels best
		if (input.up)    cam.position.y += cam.scrollSpeed * cam.zoom * delta;
		if (input.down)  cam.position.y -= cam.scrollSpeed * cam.zoom * delta;
		if (input.left)  cam.position.x -= cam.scrollSpeed * cam.zoom * delta;
		if (input.right) cam.position.x += cam.scrollSpeed * cam.zoom * delta;
		if (cam.position.x > camMaxX)	cam.position.x = camMaxX;
		if (cam.position.y > camMaxY)	cam.position.y = camMaxY;
		if (cam.position.x < camMinX)	cam.position.x = camMinX;
		if (cam.position.y < camMinY)	cam.position.y = camMinY;


	}


	void WorldSystem::onKeyCallback(GLFWwindow* /* window */, int key, int /* scancode */, int action, int /* mods */) noexcept {
		CameraInput& input = registry->cameraInputs.get(registry->getCamera());
		Entity hero = registry->animations.entities.front();
		auto& animComp = registry->animations.get(hero);
		auto* step = this->gameState->getCurrentTutorialStep();
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
				case GLFW_KEY_F5:
					if (action == GLFW_PRESS) {
						testMovement = !testMovement;
						fmt::println("They hero should be moving now!");
						break;
					}
					// ------------------------------------------------------------
					break;

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
						// if current step is BUILD_SETTLEMENT -> complete step
						if (step && step->id == TutorialStepId::BUILD_SETTLEMENT) {
							this->gameState->completeCurrentTutorialStep();
						}
    					break;
    				case GLFW_KEY_B:
    					this->isRoadPreviewActive = !this->isRoadPreviewActive;
                        fmt::println("Toggled Road Preview: {}", this->isRoadPreviewActive);
    					if (this->isRoadPreviewActive) {
    						this->isSettlementPreviewActive = false;
    					}
						// if current step is BUILD_ROAD -> complete step
						if (step && step->id == TutorialStepId::BUILD_ROAD) {
							this->gameState->completeCurrentTutorialStep();
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

	void WorldSystem::onMouseButtonCallback(GLFWwindow*, int button, int action, int /* mods */) noexcept {
		auto* step = this->gameState->getCurrentTutorialStep();
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			//action 1: press left mouse button
			//action 0: release left mouse button
			fmt::println("LMB pressed, action: {}", action);
			// Update Tutorial if finished
			if ((step && step->id == TutorialStepId::END) || (step && step->id == TutorialStepId::WELCOME && action == GLFW_PRESS)) {
				this->gameState->completeCurrentTutorialStep();
			}
		} else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			//action 1: press right mouse button
			//action 0: release right mouse button
			fmt::println("RMB pressed, action: {}", action);
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
		if (cam.zoom > cam.zoomMaxValue)	cam.zoom = cam.zoomMaxValue;
		if (cam.zoom < cam.zoomMinValue)	cam.zoom = cam.zoomMinValue;
		fmt::println("Zoom now: {}", cam.zoom);
	}


}
