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

		// texture init	// TODO: update to right ressources
		self.woodTexture = Texture::init(assets::Texture::RESSOURCE_WOOD);
		self.stoneTexture = Texture::init(assets::Texture::RESSOURCE_STONE);
		self.clayTexture = Texture::init(assets::Texture::RESSOURCE_CLAY);
		self.woolTexture = Texture::init(assets::Texture::RESSOURCE_WOOL);
		self.grainTexture = Texture::init(assets::Texture::RESSOURCE_GRAIN);

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

		// Initial Hud Settings
		self.hudPos = {window->getWindowExtent().x * 0.02f, window->getWindowExtent().y * 0.02f}; // hud starts 2% right and 2% of the bottom
		self.hudSize = {
			window->getWindowExtent().x * 0.96f, // 2% space to the right corner
			window->getWindowExtent().y * 0.08f	 // 6% in height starting from 2% window height
		};
		// End Turn Button
		float paddingX = self.hudSize.x * 0.02f; // ensure black hud layout below endTurn button
		self.endTurnButton.w = self.hudSize.x * 0.20f;
		self.endTurnButton.h = self.hudSize.y * 0.7f;
		self.endTurnButton.x = self.hudPos.x + self.hudSize.x - self.endTurnButton.w - paddingX;
		self.endTurnButton.y = self.hudPos.y + (self.hudSize.y - self.endTurnButton.h) / 2.0f;

		// Icon Size init
		float scaleX = self.viewport.size.x / self.DEFAULT_WIDTH;
		float scaleY = self.viewport.size.y / self.DEFAULT_HEIGHT;
		float scale = std::min(scaleX, scaleY);

		// icons relative to hud size
		self.iconSize = self.hudSize.y * 0.8f;
		self.iconPadding = self.iconSize + 8.0f * scale;

		// icons left centered in hud
		self.iconPos = {
			self.hudPos.x + 20.0f * scale,
			self.hudPos.y + (self.hudSize.y - self.iconSize) / 2.0f};

		return self;
	}

	void RenderHudSystem::deinit() noexcept {
		rectShader.deinit();
		textureShader.deinit();
	}

	void RenderHudSystem::step(float /*dt*/) noexcept {
		RenderTextSystem* textSystem = registry->getSystem<RenderTextSystem>();
		if (textSystem) {
			// scale text size for fullscreen
			float scaleX = viewport.size.x / DEFAULT_WIDTH;
			float scaleY = viewport.size.y / DEFAULT_HEIGHT;
			float scale = std::min(scaleX, scaleY);
			// Render Tutorial
			if (gameState->isTutorialActive()) {
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
				// Tutorial box
				if (step->renderBox) {
					renderRectBox(pos, rectBoxSize, {0.0f, 0.0f, 0.0f});
				}

				// center text in the middle of the box
				float offset = 40.0f * scale;
				glm::vec2 textPos = {
					pos.x + (rectBoxSize.x - textSize.x) / 2.0f,
					pos.y + (rectBoxSize.y + textSize.y) / 2.0f - offset};
				// Tutorial-Text
				textSystem->renderText(step->text, textPos, scale, {1.f, 1.f, 1.f});
			}
			// Render Box for HUD
			renderRectBox(hudPos, hudSize, {0.0f, 0.0f, 0.0f});

			// Render icons and mount in HUD
			// get current player and ressources
			Player& player = *gameState->getPlayer(gameState->getCurrentPlayerId());
			std::map<types::TileType, int> resources = player.getResources();
			resourceIconsWithAmount = {
				{woodTexture, resources[types::TileType::FOREST]},
				{stoneTexture, resources[types::TileType::MOUNTAIN]},
				{clayTexture, resources[types::TileType::CLAY]},
				{woolTexture, resources[types::TileType::GRASS]},
				{grainTexture, resources[types::TileType::FIELD]},
			};

			// start at the left side of the hud with an offset defined in init/onResizeCallback
			float x = iconPos.x;
			float hudCenterY = hudPos.y + hudSize.y * 0.5f;
			float hudTextOffset = 8.f * scale;

			// render ressource icons + amount
			for (std::pair<Texture, int>& pair : resourceIconsWithAmount) {
				Texture& tex = pair.first;
				int& amount = pair.second;
				// display icons (y centered on hud)
				drawSprite(tex, {x, hudCenterY - iconSize / 2.0f}, {iconSize, iconSize}, {1.f, 1.f, 1.f});

				// display amount of ressource
				std::string amountStr = std::to_string(amount);
				glm::vec2 textSize = textSystem->measureText(amountStr, scale * 1.2f);
				glm::vec2 textPos = {
					x + iconPadding,
					hudCenterY - textSize.y / 2.0f + hudTextOffset};
				textSystem->renderText(amountStr, textPos, scale * 1.2f, {1.f, 1.f, 1.f});

				// prepare next writing position
				x += iconPadding * 1.3f + textSize.x;
			}
			// display round
			std::string roundText = "Round: " + std::to_string(gameState->getRoundNumber());
			glm::vec2 roundTextSize = textSystem->measureText(roundText, scale * 1.2f);
			glm::vec2 roundTextPos = {
				x + iconPadding,
				hudCenterY - roundTextSize.y / 2.0f + hudTextOffset};
			textSystem->renderText(roundText, roundTextPos, scale * 1.2f, {1.f, 1.f, 1.f});

			// End Turn Button
			renderRectBox({endTurnButton.x, endTurnButton.y}, {endTurnButton.w, endTurnButton.h}, {0.0f, 0.0f, 1.0f});
			glm::vec2 textSizeEndTurn = textSystem->measureText("End Turn", scale * 0.9f);
			glm::vec2 buttonTextPos = {
				endTurnButton.x + (endTurnButton.w - textSizeEndTurn.x) / 2.0f,
				endTurnButton.y + (endTurnButton.h - textSizeEndTurn.y) / 2.0f + textSizeEndTurn.y * 0.15 // shift slightly up
			};
			textSystem->renderText("End Turn", buttonTextPos, scale * 0.9f, {1.f, 1.f, 1.f});
		}
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
		return wasEndTurnClicked(mouse, button, action) ? true : false; // endTurn() will be called by Application
	}

	void RenderHudSystem::onResizeCallback(GLFWwindow* /*window*/, int width, int height) noexcept {

		this->viewport.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
		hudSize = {
			window->getWindowExtent().x * 0.96f, // 2% space to the right corner
			window->getWindowExtent().y * 0.08f	 // 6% in height starting from 2% window height
		};
		hudPos = {window->getWindowExtent().x * 0.02f, window->getWindowExtent().y * 0.02f}; // hud starts 2% right and 2% of the bottom
		float paddingX = hudSize.x * 0.02f;													 // ensure black hud layout below endTurn button
		endTurnButton.w = hudSize.x * 0.20f;
		endTurnButton.h = hudSize.y * 0.7f;
		endTurnButton.x = hudPos.x + hudSize.x - endTurnButton.w - paddingX;
		endTurnButton.y = hudPos.y + (hudSize.y - endTurnButton.h) / 2.0f;

		// Icons
		float scaleX = viewport.size.x / DEFAULT_WIDTH;
		float scaleY = viewport.size.y / DEFAULT_HEIGHT;
		float scale = std::min(scaleX, scaleY);

		iconSize = hudSize.y * 0.8f;
		iconPadding = iconSize + 8.0f * scale;
		iconPos = {
			hudPos.x + 20.0f * scale,
			hudPos.y + (hudSize.y - iconSize) / 2.0f};

	}

} // namespace df
