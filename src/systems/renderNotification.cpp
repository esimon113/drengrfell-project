#include "renderNotification.h"

namespace df {
	RenderNotificationSystem RenderNotificationSystem::init(Window* window, Registry* registry) noexcept {
		RenderNotificationSystem self;
		self.window = window;
		self.registry = registry;

		self.viewport.origin = glm::uvec2(0);
		self.viewport.size = self.window->getWindowExtent();

		self.rectShader = Shader::init(assets::Shader::hud).value();
		// Textures for Background/Buttons
		self.textureShader = Shader::init(assets::Shader::menu).value(); 
		self.notificationBackgroundTexture = Texture::init(assets::Texture::NOTIFICATIONS_BACKGROUND);
		self.notificationButtonTexture = Texture::init(assets::Texture::NOTIFICATIONS_BUTTON); 

		// Quad rectangle 1x1, transformed via model later
		float quad[] = {
			0.f, 0.f,
			1.f, 0.f,
			1.f, 1.f,
			0.f, 1.f};

		glGenVertexArrays(1, &self.quadVao);
		glBindVertexArray(self.quadVao);

		glGenBuffers(1, &self.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, self.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

		glBindVertexArray(0);

		return self;
	}

	void RenderNotificationSystem::deinit() noexcept {
		rectShader.deinit();
	}

	void RenderNotificationSystem::reset() noexcept {
		active = false;
		title.clear();
		message.clear();
		buttons.clear();
		buttonTexts.clear();
	}

	void RenderNotificationSystem::showNotification(const std::string& givenTitle, const std::string& givenMessage, const std::vector<std::string>& givenButtonTexts) {
		auto* textSystem = registry->getSystem<RenderTextSystem>();
		if (!textSystem)
			return;

		title = givenTitle;
		message = givenMessage;
		buttonTexts = givenButtonTexts;
		buttons.clear();

		// scale
		float scaleX = viewport.size.x / DEFAULT_WIDTH;
		float scaleY = viewport.size.y / DEFAULT_HEIGHT;
		scale = std::min(scaleX, scaleY);

		// use same padding as hud for now
		paddingX = 20.f * scale;
		paddingY = 20.f * scale;

		// Measure text
		glm::vec2 titleSize = textSystem->measureText(title, scale * 1.2f);	// ensure a little more space for title
		glm::vec2 messageSize = textSystem->measureText(message, scale);

		float maxTextWidth = std::max(titleSize.x, messageSize.x);

		// compute button position
		float totalButtonWidth = 0.f;
		float buttonHeight = 60.f * scale;

		// save all buttons
		for (std::string text : buttonTexts) {
			glm::vec2 size = textSystem->measureText(text, scale);
			float width = size.x + paddingX * 2.f;	// ensure padding from both sides

			// add button
			buttons.push_back({text, 0.f, 0.f, width, buttonHeight});

			totalButtonWidth += width + paddingX;
		}

		// remove last padding
		if (!buttons.empty())
			totalButtonWidth -= paddingX;

		// compute new maxTextWidth based on text and buttons
		maxTextWidth = std::max(maxTextWidth, totalButtonWidth);

		// compute backgroundbox size
		float renderedButtons = 0.f;
		// if buttons are rendered add their height to the box
		if (buttons.empty()){
			renderedButtons = 0.f;
		}
		else {
			renderedButtons = buttonHeight;
		}
		// compute box size
		boxSize.x = maxTextWidth + paddingX * 2.f;	// ensure padding on both sides
		boxSize.y = titleSize.y + messageSize.y + paddingY * 3.f + renderedButtons; // ensure enough space between title message and buttons

		// center box in the middle of the screen
		boxPos = {
			(viewport.size.x - boxSize.x) / 2.0f,
			(viewport.size.y - boxSize.y) / 2.0f
		};

		// center buttons horizontally
		float buttonX = boxPos.x + (boxSize.x - totalButtonWidth) / 2.0f;
		float buttonY = boxPos.y + paddingY;

		for (Button& btn : buttons) {
			btn.x = buttonX;
			btn.y = buttonY;
			buttonX += btn.w + paddingX;	// add current width for next button
		}
		// set visible
		active = true;
	}

	void RenderNotificationSystem::step(float /*dt*/) noexcept {
		if (!active)
			return;

		auto* textSystem = registry->getSystem<RenderTextSystem>();
		if (!textSystem)
			return;

		// background texture
		drawSprite(notificationBackgroundTexture, boxPos, boxSize, {1.f, 1.f, 1.f});

		// title
		glm::vec2 titleSize = textSystem->measureText(title, scale * 1.2f); // ensure a little more space for title therefore scale *1.2f
		glm::vec2 titlePos{
			boxPos.x + (boxSize.x - titleSize.x) / 2.0f,	// center title
			boxPos.y + boxSize.y - titleSize.y - paddingY + titleSize.y * 0.40f};	// manually tweaked to look good
		textSystem->renderText(title, titlePos, scale * 1.2f, {0.f, 0.f, 0.f}); // ensure a little more space for title therefore scale *1.2f

		// message
		glm::vec2 messageSize = textSystem->measureText(message, scale);
		float messageStartY = boxPos.y + boxSize.y - paddingY * 3.f - titleSize.y;	// manually tweaked to look good

		glm::vec2 messagePos{
			boxPos.x + (boxSize.x - messageSize.x) / 2.0f,	// center message
			messageStartY};

		textSystem->renderText(message, messagePos, scale, {0.f, 0.f, 0.f});

		// buttons
		for (Button& btn : buttons) {
			// render texture for each button
			drawSprite(notificationButtonTexture, {btn.x, btn.y}, {btn.w, btn.h}, {1.f, 1.f, 1.f});

			glm::vec2 textSize = textSystem->measureText(btn.text, scale);
			glm::vec2 textPos{
				btn.x + (btn.w - textSize.x) / 2.0f,	// center text
				btn.y + (btn.h - textSize.y) / 2.0f + textSize.y * 0.15f}; // shift slightly up to look good

			textSystem->renderText(btn.text, textPos, scale, {0.f, 0.f, 0.f});
		}
	}

	std::string RenderNotificationSystem::onMouseButton(glm::vec2 mouse, int button, int action) noexcept {
		// if nothing to show or not klicked the left mouse button return empty string
		if (!active || button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
			return "";

		// check if any button pressed
		for (Button& btn : buttons) {
			if (mouse.x >= btn.x && mouse.x <= btn.x + btn.w &&
				mouse.y >= btn.y && mouse.y <= btn.y + btn.h) {
				active = false;
				return btn.text;
			}
		}
		return "";
	}

	void RenderNotificationSystem::onResizeCallback(GLFWwindow*, int width, int height) noexcept {
		viewport.size = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)};

		// redraw the notification with given width/height
		if (active) {
			showNotification(title, message, buttonTexts);
		}
	}

	void RenderNotificationSystem::renderBox(glm::vec2 pos, glm::vec2 size, glm::vec3 color) const noexcept {
		glm::mat4 projection = glm::ortho(
			0.f, static_cast<float>(viewport.size.x), 0.f, static_cast<float>(viewport.size.y), -1.f, 1.f
		);

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
	void RenderNotificationSystem::drawSprite(Texture& tex, glm::vec2 pos, glm::vec2 size, glm::vec3 color) const noexcept {
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

} // namespace df
