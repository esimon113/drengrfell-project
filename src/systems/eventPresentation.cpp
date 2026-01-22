#include "eventPresentation.h"


namespace df {

	EventPresentationSystem EventPresentationSystem::init(Window* window, Registry* registry, EventBus* eventBus) noexcept {
		EventPresentationSystem self;
		self.window = window;
		self.registry = registry;
		self.eventBus = eventBus;


		self.dimShader = Shader::init(assets::Shader::dimScreen).value();
		self.imgShader = Shader::init(assets::Shader::menu).value();

		self.initQuadBuffers();

		self.calcImgLayout();
		return self;
	}

	void EventPresentationSystem::deinit() noexcept {
		dimShader.deinit();
		imgShader.deinit();
	}

	void EventPresentationSystem::reset() noexcept {
		endEvent();
	}

	void EventPresentationSystem::onResizeCallback(GLFWwindow*, int width, int height) noexcept {
		// Update viewport
		glViewport(0, 0, width, height);

		calcImgLayout();
	}

	void EventPresentationSystem::calcImgLayout() noexcept {
		glm::uvec2 extent = window->getWindowExtent();
		float winW = static_cast<float>(extent.x);
		float winH = static_cast<float>(extent.y);

		float aspect = winW / winH;

		// size of the image, relative to window size
		float imgWidth = 0.29f;
		float imgHeight = imgWidth * 1.5f * aspect; // img is 1.5 times higher than wide, so it fits well beside the notification


		imgSize = glm::vec2(winW * imgWidth, winH * imgHeight);
		imgPosRight = glm::vec2((winW - imgSize.x), (winH - imgSize.y) * 0.5f);
		imgPosLeft = glm::vec2(0, (winH - imgSize.y) * 0.5f);
	}

	void EventPresentationSystem::step(float dt) noexcept {
		if (!currentEvent)	// only show the event while an event is active, i.e. when an event was started via presentEvent and not yet ended via endEvent
			return;

		auto& event = *currentEvent;
		event.timer += dt;
		dimScreenStep(dt);
		if (event.stage > 0) {	// the image is first rendered in stage 1 of the event, but should stay during the whole rest of the event
			renderImage();
		}

		presentEventStages();
	}

	void EventPresentationSystem::presentEventStages() noexcept {
		RenderNotificationSystem* notification = registry->getSystem<RenderNotificationSystem>();

		auto& event = *currentEvent;
		switch (event.stage) {
		case 0: // darken the screen
			if (event.timer >= dimTime) {
				event.stage = 1;
				event.timer = 0.0f;
				fmt::println("Switching to event stage 1");
			}
			break;
		case 1: // event sound is played and image rendered
			emitSoundSignal(event.eventType);
			event.stage = 2;
			fmt::println("Switching to event stage 2");
			break;
		case 2: // waiting a bit before showing the notification
			if (event.timer >= 1.0f) {
				event.stage = 3;
				fmt::println("Switching to event stage 3");
				event.timer = 0.0f;
			}
			break;
		case 3: // notification is shown
			notification->showNotification(event.title, event.message, event.buttonTexts);
			event.stage = 4;
			fmt::println("Switching to event stage 4");
			break;
		case 4: // notification stays on screen until event ends
			break;
		}
	}

	// Shows a notification together with sound and picture, depending on the eventType
	void EventPresentationSystem::presentEvent(const std::string& title, const std::string& message, const std::vector<std::string>& buttonTexts, types::EventType eventType, std::string /* image */, bool locking) noexcept {
		currentEvent = ActiveEvent{title, message, buttonTexts, eventType, 0.0f, 0, locking};
		eventBus->eventPoppedUp.emit();
		loadImageTexture(eventType);
	}

	void EventPresentationSystem::endEvent() noexcept {
		currentEvent.reset();
		dimProgress = 0.0f;
	}

	void EventPresentationSystem::emitSoundSignal(types::EventType event) noexcept {
		switch (event) {
		case types::EventType::HAZARD_BEAR:
			eventBus->hazardBearEncountered.emit();
			break;
		case types::EventType::HAZARD_BLIZZARD:
			eventBus->hazardBlizzardEncountered.emit();
			break;
		case types::EventType::HAZARD_MUD:
			eventBus->hazardMudEncountered.emit();
			break;
		case types::EventType::HAZARD_ROCKSLIDE:
			eventBus->hazardRockslideEncountered.emit();
			break;
		}
	}

	void EventPresentationSystem::loadImageTexture(types::EventType event) noexcept {
		switch (event) {
		case types::EventType::HAZARD_BEAR:
			imgTexture = Texture::init(assets::Texture::HAZARD_BEAR);
			break;
		case types::EventType::HAZARD_BLIZZARD:
			imgTexture = Texture::init(assets::Texture::HAZARD_BLIZZARD);
			break;
		case types::EventType::HAZARD_MUD:
			imgTexture = Texture::init(assets::Texture::HAZARD_MUD);
			break;
		case types::EventType::HAZARD_ROCKSLIDE:
			imgTexture = Texture::init(assets::Texture::HAZARD_ROCKSLIDE);
			break;
		}
	}


	void EventPresentationSystem::dimScreenStep(float dt) noexcept {
		if (dimProgress < 1.0f) {
			dimProgress += dt / dimTime;
		} else {
			dimProgress = 1.0f;
		}

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		glBindVertexArray(quad_vao);

		dimShader.use()
			.setFloat("progress", dimProgress)
			.setFloat("maxDarkness", 0.7f);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	void EventPresentationSystem::renderImage() noexcept {
		if (!window)
			return;

		glm::uvec2 extent = window->getWindowExtent();
		float winW = static_cast<float>(extent.x);
		float winH = static_cast<float>(extent.y);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);

		// full-screen orthographic projection in screen pixels, origin bottom-left
		glm::mat4 projection = glm::ortho(0.0f, winW, 0.0f, winH, -1.0f, 1.0f);
		glm::mat4 view = glm::identity<glm::mat4>();

		glBindVertexArray(quad_vao);

		// helper function for drawing each element
		auto drawSprite = [&](Texture& tex, glm::vec2 pos, glm::vec2 size, glm::vec3 color) {
			glm::mat4 model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(pos, 0.0f));
			model = glm::scale(model, glm::vec3(size, 1.0f));

			tex.bind(0);
			imgShader.use()
				.setMat4("model", model)
				.setMat4("view", view)
				.setMat4("projection", projection)
				.setSampler("sprite", 0)
				.setVec3("fcolor", color);

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		};

		drawSprite(imgTexture, imgPosRight, imgSize, glm::vec3(1.0f));
		drawSprite(imgTexture, imgPosLeft, imgSize, glm::vec3(1.0f));

		glBindVertexArray(0);
	}

	void EventPresentationSystem::initQuadBuffers() noexcept {
		float quadVertices[] = {
			// positions (x,y), texture-coords (u,v)
			0.0f, 0.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 1.0f, 0.0f,
			1.0f, 1.0f, 1.0f, 1.0f,
			0.0f, 1.0f, 0.0f, 1.0f};
		// build square from two triangles
		constexpr GLuint quadIndices[] = {0, 1, 2, 2, 3, 0};

		glGenVertexArrays(1, &quad_vao);
		glBindVertexArray(quad_vao);

		glGenBuffers(1, &quad_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

		glGenBuffers(1, &quad_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

		// layout: location 0 = positions (x,y), location 1 = texture-coords (u,v)
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

		glBindVertexArray(0);
	}


} // namespace df
