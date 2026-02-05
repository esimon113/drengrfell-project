#include "renderHud.h"
#include <iostream>

namespace df {

	RenderHudSystem RenderHudSystem::init(Window* window, Registry* registry, std::shared_ptr<GameState> gameState) noexcept {
		RenderHudSystem self;

		self.window = window;
		self.registry = registry;
		self.gameState = gameState;

		self.viewport.origin = glm::uvec2(0);
		self.viewport.size = self.window->getWindowExtent();

		glm::uvec2 extent = self.viewport.size;

		// shader init
		self.rectShader = Shader::init(assets::Shader::hud).value();
		self.textureShader = Shader::init(assets::Shader::menu).value();

		// texture init
		self.hudBackgroundTexture = Texture::init(assets::Texture::HUD_BACKGROUND);
		self.tutorialTexture = Texture::init(assets::Texture::HUD_BACKGROUND);	// use same as hud for now
		// buttom hud ressources
		self.woodTexture = Texture::init(assets::Texture::RESSOURCE_WOOD);
		self.stoneTexture = Texture::init(assets::Texture::RESSOURCE_STONE);
		self.clayTexture = Texture::init(assets::Texture::RESSOURCE_CLAY);
		self.woolTexture = Texture::init(assets::Texture::RESSOURCE_WOOL);
		self.grainTexture = Texture::init(assets::Texture::RESSOURCE_GRAIN);
		self.heroPointsTexture = Texture::init(assets::Texture::RESSOURCE_HERO_POINTS);

		// texture init	side hud buttons
		self.tradeTexture = Texture::init(assets::Texture::SIDE_HUD_TRADE_BUTTON);
		self.questTexture = Texture::init(assets::Texture::SIDE_HUD_QUEST_BUTTON);
		self.keybindingsTexture = Texture::init(assets::Texture::SIDE_HUD_KEYBINDINGS_BUTTON);
		self.costTexture = Texture::init(assets::Texture::SIDE_HUD_COST_BUTTON);
		self.hudEndTurnButtonTexture = Texture::init(assets::Texture::HUD_END_TURN_BUTTON);
		self.heroPointsSideHudTexture = Texture::init(assets::Texture::SIDE_HUD_HERO_POINTS_BUTTON);

		// Viewport
		glViewport(0, 0, extent.x, extent.y);

		// Quad rectangle 1x1, transformed via model later
		float quad[] = {
			// x, y, u, v
			0.f, 0.f, 0.f, 0.f,
			1.f, 0.f, 1.f, 0.f,
			1.f, 1.f, 1.f, 1.f,
			0.f, 1.f, 0.f, 1.f
		};

		// Shader
		glGenVertexArrays(1, &self.quadVao);
		glBindVertexArray(self.quadVao);

		glGenBuffers(1, &self.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, self.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

		// Texture
		// layout: location 0 = positions (x,y), location 1 = texture-coords (u,v)
		glEnableVertexAttribArray(0);	// position
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);	// uv
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

		glBindVertexArray(0);
		self.scaleHud();

		return self;
	}

	/*
	* scale the hud and the side hud after initialisation/resize call
	*/
	void RenderHudSystem::scaleHud() noexcept {
		// INITIAL BOTTOM HUD SETTINGS
		hudPos = {window->getWindowExtent().x * 0.02f, window->getWindowExtent().y * 0.02f}; // hud starts 2% right and 2% of the bottom
		hudSize = {
			window->getWindowExtent().x * 0.96f, // 2% space to the right corner
			window->getWindowExtent().y * 0.08f	 // 6% in height starting from 2% window height
		};
		// End Turn Button
		float paddingX = hudSize.x * 0.01f; // ensure black hud layout below endTurn button
		endTurnButton.w = hudSize.x * 0.12f;
		endTurnButton.h = hudSize.y * 0.85f;
		endTurnButton.x = hudPos.x + hudSize.x - endTurnButton.w - paddingX;
		endTurnButton.y = hudPos.y + (hudSize.y - endTurnButton.h) / 2.0f;

		// Icon Size init
		float scaleX = viewport.size.x / DEFAULT_WIDTH;
		float scaleY = viewport.size.y / DEFAULT_HEIGHT;
		float scale = std::min(scaleX, scaleY);

		// icons relative to hud size
		iconSize = hudSize.y * 0.8f;
		iconPadding = iconSize + 8.0f * scale;

		// icons left centered in hud
		iconPos = {
			hudPos.x + 20.0f * scale,
			hudPos.y + (hudSize.y - iconSize) / 2.0f};

		// SIDE HUD FOR UI INTERACTIONS
		// side hud size (invisible just for orientation)
		sideHudSize = {
			viewport.size.x * 0.08f, // 8% width
			viewport.size.y * 0.80f  // 80% height
		};

		// size hud pos (invisible just for orientation)
		sideHudPos = {
			viewport.size.x - sideHudSize.x - 10.f * scale, // right side with little padding
			viewport.size.y - sideHudSize.y - 40.f * scale  // right upper corner with little padding
		};

		// Buttons
		float buttonHeight = sideHudSize.y * 0.1f;
		float buttonPadding = buttonHeight * 0.2f;

		float buttonYPos = sideHudPos.y + sideHudSize.y - buttonHeight - buttonPadding;

		// define buttons position and size
		sideButtons = {
			{sideHudPos.x + 10.f * scale, buttonYPos, buttonHeight, buttonHeight, "Quest", questTexture},
			{sideHudPos.x + 10.f * scale, buttonYPos - (buttonHeight + buttonPadding), buttonHeight, buttonHeight, "Trade", tradeTexture},
			{sideHudPos.x + 10.f * scale, buttonYPos - 2 * (buttonHeight + buttonPadding), buttonHeight, buttonHeight, "Cost", costTexture},
			{sideHudPos.x + 10.f * scale, buttonYPos - 3 * (buttonHeight + buttonPadding), buttonHeight, buttonHeight, "Keybindings", keybindingsTexture},
			{sideHudPos.x + 10.f * scale, buttonYPos - 4 * (buttonHeight + buttonPadding), buttonHeight, buttonHeight, "HeroPoints", heroPointsSideHudTexture},
		};
	}

	void RenderHudSystem::deinit() noexcept {
		rectShader.deinit();
		textureShader.deinit();
	}

	void RenderHudSystem::step(float /*dt*/) noexcept {
		RenderTextSystem* textSystem = registry->getSystem<RenderTextSystem>();
		EventPresentationSystem* eventSystem = registry->getSystem<EventPresentationSystem>();
		RenderSettlementMenuSystem* settlementSystem = registry->getSystem<RenderSettlementMenuSystem>();
		if (textSystem) {
			// scale text size for fullscreen
			float scaleX = viewport.size.x / DEFAULT_WIDTH;
			float scaleY = viewport.size.y / DEFAULT_HEIGHT;
			float scale = std::min(scaleX, scaleY);
			// Render Tutorial
			if (gameState->isTutorialActive() && !eventSystem->currentEvent && !settlementSystem->isActive()) {
				// get next tutorial step
				TutorialStep* step = gameState->getCurrentTutorialStep();
				if (!step)
					return;
				// box position/size
				float boxPosPaddingX = 20.f * scale;
				float boxPosPaddingY = 20.f * scale;
				glm::vec2 textSize = textSystem->measureText(step->text, scale);
				glm::vec2 rectBoxSize = {
					textSize.x + boxPosPaddingX,
					textSize.y + boxPosPaddingY};
				glm::vec2 pos = {10.0f, window->getWindowExtent().y - rectBoxSize.y - 10.0f}; // box position always top left
				if (step->id == TutorialStepId::WELCOME)
					// center box in the middle of the screen
					pos = {(viewport.size.x - rectBoxSize.x) / 2.0f, (viewport.size.y - rectBoxSize.y) / 2.0f}; // render first tutorial "into your face" like requested in cross-play session
				// Tutorial box
				if (step->renderBox) {
					drawSprite(tutorialTexture, pos, rectBoxSize, {1.f, 1.f, 1.f});
					//renderRectBox(pos, rectBoxSize, {0.0f, 0.0f, 0.0f});
				}

				// center text in the middle of the box
				float offset = 40.0f * scale;
				glm::vec2 textPos = {
					pos.x + (rectBoxSize.x - textSize.x) / 2.0f,
					pos.y + (rectBoxSize.y + textSize.y) / 2.0f - offset};
				// Tutorial-Text
				textSystem->renderText(step->text, textPos, scale, {1.f, 1.f, 1.f});
			}
			// Render Box for HUD (old version)
			//renderRectBox(hudPos, hudSize, {0.0f, 0.0f, 0.0f});

			// render hud background texture
			drawSprite(hudBackgroundTexture, hudPos, hudSize, {1.f, 1.f, 1.f});

			// Render icons and mount in HUD
			// get current player and ressources
			Player& player = *gameState->getPlayer(gameState->getCurrentPlayerId());
			std::map<types::TileType, int> resources = player.getResources();
			resourceIconsWithAmount = {
				{woodTexture, resources[types::TileType::FOREST], hudResourceColors[0]},
				{stoneTexture, resources[types::TileType::MOUNTAIN], hudResourceColors[1]},
				{clayTexture, resources[types::TileType::CLAY], hudResourceColors[2]},
				{woolTexture, resources[types::TileType::GRASS], hudResourceColors[3]},
				{grainTexture, resources[types::TileType::FIELD], hudResourceColors[4]},
			};

			// start at the left side of the hud with an offset defined in init/onResizeCallback
			float x = iconPos.x;
			float hudCenterY = hudPos.y + hudSize.y * 0.5f;
			float hudTextOffset = 8.f * scale;

			// render ressource icons + amount
			for (std::tuple<Texture, int, glm::vec3>& triple : resourceIconsWithAmount) {
				Texture& tex = get<0>(triple);
				int& amount = get<1>(triple);
				// display icons (y centered on hud)
				drawSprite(tex, {x, hudCenterY - iconSize / 2.0f}, {iconSize, iconSize}, {1.f, 1.f, 1.f});

				// display amount of ressource
				std::string amountStr = std::to_string(amount);
				glm::vec2 textSize = textSystem->measureText(amountStr, scale * 1.2f);
				glm::vec2 textPos = {
					x + iconPadding,
					hudCenterY - textSize.y / 2.0f + hudTextOffset};

				textSystem->renderText(amountStr, textPos, scale * 1.2f, get<2>(triple));

				// prepare next writing position
				x += iconPadding * 1.3f + textSize.x;
			}
			// display round + hero points
			int heroPoints = player.getHeroPoints();

			glm::vec2 heroIconSize = {iconSize, iconSize}; 
			std::string heroPointsText = std::to_string(heroPoints);
			glm::vec2 heroPointsTextSize = textSystem->measureText(heroPointsText, scale * 1.2f);

			std::string roundText = "Round: " + std::to_string(gameState->getRoundNumber());
			glm::vec2 roundTextSize = textSystem->measureText(roundText, scale * 1.2f);

			// compute max width
			float textPadding = 120.f * scale; // padding between hero points and rounds
			float totalWidth = heroIconSize.x + hudTextOffset + heroPointsTextSize.x + textPadding + roundTextSize.x;

			// center hero points and round count
			float centerX = (x + endTurnButton.x) / 2.0f;
			float startX = centerX - totalWidth / 2.0f;
			float textY = hudCenterY - heroPointsTextSize.y / 2.0f + hudTextOffset;

			// display hero points texture + amount
			drawSprite(heroPointsTexture, {startX, hudCenterY - iconSize / 2.0f}, heroIconSize, {1.f, 1.f, 1.f});
			textSystem->renderText(heroPointsText, {startX + iconSize + hudTextOffset, textY}, scale * 1.2f, {1.f, 1.f, 1.f});

			// display rounds
			textSystem->renderText(roundText, {startX + iconSize + hudTextOffset + heroPointsTextSize.x + textPadding, textY}, scale * 1.2f, {1.f, 1.f, 1.f});

			// End Turn Button
			// hover detection (update hovered flags)
			glm::dvec2 cursor = window->getCursorPosition();
			// window returns screen coords with origin top-left
			float mouseX = static_cast<float>(cursor.x);
			float mouseY = static_cast<float>(cursor.y);

			// convert to bottom-left origin, because the layout uses bottom-left
			glm::uvec2 extent = window->getWindowExtent();
			mouseY = static_cast<float>(extent.y) - mouseY;
			if (isMouseOverEndTurn({mouseX, mouseY})) {
				drawSprite(hudEndTurnButtonTexture, {endTurnButton.x, endTurnButton.y}, {endTurnButton.w, endTurnButton.h}, {1.3f, 1.3f, 1.3f});
			} else {
				drawSprite(hudEndTurnButtonTexture, {endTurnButton.x, endTurnButton.y}, {endTurnButton.w, endTurnButton.h}, {1.f, 1.f, 1.f});
			}
			
			// SIDE HUD FOR UI INTERACTIONS
			// check if any event or settlement menu active, if not display ui controlls
			if (!eventSystem->currentEvent && !settlementSystem->isActive()) {
				// side hud background box
				//renderRectBox(sideHudPos, sideHudSize, {0.0f, 0.0f, 0.0f});

				// side hud buttons
				for (SideHudButton& btn : sideButtons) {
					// render icons
					if (btn.icon) {
						if (isMouseOverSideHudButton(btn, {mouseX, mouseY})) {
							drawSprite(btn.icon, {btn.x, btn.y}, {btn.w, btn.h}, {1.3f, 1.3f, 1.3f});
						} else {
							drawSprite(btn.icon, {btn.x, btn.y}, {btn.w, btn.h}, {1.f, 1.f, 1.f});
						}
					}
				}
			}

		}
	}

	bool RenderHudSystem::isMouseOverSideHudButton(const SideHudButton& btn, glm::vec2 mouse) const noexcept {
			return mouse.x >= btn.x && mouse.x <= btn.x + btn.w &&
				   mouse.y >= btn.y && mouse.y <= btn.y + btn.h;
		}

	std::string RenderHudSystem::getSideHudButtonClicked(glm::vec2 mouse, int button, int action) const noexcept {
		if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
			return "";

		for (const SideHudButton& btn : sideButtons) {
			if (isMouseOverSideHudButton(btn, mouse))
				return btn.label;
		}
		return "";
	}

	void RenderHudSystem::reset() noexcept {
		RenderTextSystem* textSystem = registry->getSystem<RenderTextSystem>();
		if (textSystem) {
			textSystem->renderText(" ", {0.0f, 0.0f}, 0.5f, {1.0f, 1.0f, 1.0f});
		}
	}

	void RenderHudSystem::renderRectBox(glm::vec2 pos, glm::vec2 size, glm::vec3 color) const noexcept {

		const float width = viewport.size.x;
		const float height = viewport.size.y;

		glm::mat4 projection = glm::ortho(0.f, width, 0.f, height, -1.f, 1.f);

		glm::mat4 model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(pos, 0.f));
		model = glm::scale(model, glm::vec3(size, 1.f));

		rectShader.use()
			.setMat4("projection", projection)
			.setMat4("view", glm::identity<glm::mat4>())
			.setMat4("model", model)
			.setVec3("fcolor", color);

		glBindVertexArray(quadVao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}

	// used from mainMenu.cpp; slighlty modified
	void RenderHudSystem::drawSprite(Texture& tex, glm::vec2 pos, glm::vec2 size, glm::vec3 color) const noexcept {
		const float width = viewport.size.x;
		const float height = viewport.size.y;

		glm::mat4 projection = glm::ortho(0.f, width, 0.f, height, -1.f, 1.f);
		glm::mat4 view = glm::identity<glm::mat4>();

		glm::mat4 model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(pos, 0.f));
		model = glm::scale(model, glm::vec3(size, 1.f));

		tex.bind(0);
		textureShader.use()
			.setMat4("projection", projection)
			.setMat4("view", view)
			.setMat4("model", model)
			.setSampler("sprite", 0)
			.setVec3("fcolor", color);

		glBindVertexArray(quadVao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}


	bool RenderHudSystem::isMouseOverEndTurn(glm::vec2 mouse) const noexcept {
		return mouse.x >= endTurnButton.x &&
			mouse.x <= endTurnButton.x + endTurnButton.w &&
			mouse.y >= endTurnButton.y &&
			mouse.y <= endTurnButton.y + endTurnButton.h;
	}

	bool RenderHudSystem::wasEndTurnClicked(glm::vec2 mouse, int button, int action) const noexcept {
		bool result = button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && isMouseOverEndTurn(mouse);
		return result;
	}


	bool RenderHudSystem::onMouseButton(glm::vec2 mouse, int button, int action) noexcept {
		// bottom hud check
		if (wasEndTurnClicked(mouse, button, action))
			return true; // endTurn() will be called by Application

		// Side Hud Checks
		std::string sideButtonClick = getSideHudButtonClicked(mouse, button, action);
		if (!sideButtonClick.empty()) {
			lastSideHudButtonPressed = sideButtonClick;
			return true;
		}
		return false;
	}

	void RenderHudSystem::onResizeCallback(GLFWwindow* /*window*/, int width, int height) noexcept {

		this->viewport.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
		scaleHud();
	}

} // namespace df
