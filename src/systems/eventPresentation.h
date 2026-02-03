#pragma once

#include "registry.h"
#include "window.h"
#include "types.h"
#include "events/eventBus.h"
#include "renderNotification.h"
#include "utils/texture.h"

namespace df {
	struct ActiveEvent {
		// for the notification
		std::string title;
		std::string message;
		std::vector<std::string> buttonTexts;

		types::EventType eventType;	// for choosing the image and sound
		float timer = 0.0f;			// for timing the different event stages
		int stage = 0;				// for counting the different event stages
		bool showingImage = false;	// if the event image is on screen
		bool dimScreen = false;		// if the screen gets dimmed
	};

	class EventPresentationSystem {
	  public:
		EventPresentationSystem() = default;
		~EventPresentationSystem() = default;

		static EventPresentationSystem init(Window* window, Registry* registry, EventBus* eventBus) noexcept;
		void deinit() noexcept;
		void reset() noexcept;
		void onResizeCallback(GLFWwindow*, int width, int height) noexcept;
		void calcImgLayout() noexcept;
		void step(float dt) noexcept;

		void presentEventStages() noexcept;

		void presentEvent(const std::string& title, const std::string& message, const std::vector<std::string>& buttonTexts, types::EventType eventType) noexcept;
		void endEvent() noexcept;

		void emitSoundSignal(types::EventType event) noexcept;
		void loadImageTexture(types::EventType event) noexcept;
		void startEventMusic(types::EventType event) noexcept;
		void dimScreenStep(float dt) noexcept;
		void renderImage() noexcept;
		std::optional<ActiveEvent> currentEvent;


	  private:
		Registry* registry = nullptr;
		Window* window = nullptr;
		EventBus* eventBus = nullptr;

		Shader imgShader;
		Shader dimShader;
		GLuint quad_vao;
		GLuint quad_vbo;
		GLuint quad_ebo;
		float dimProgress = 0.0f;
		float dimAlpha = 0.9f;
		float dimTime = 1.5f;	// how long the dimming of the screen takes
		Texture imgTexture;
		glm::vec2 imgPosRight{};
		glm::vec2 imgPosLeft{};
		glm::vec2 imgSize{};

		void initQuadBuffers() noexcept;
	};
} // namespace df
