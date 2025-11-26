#pragma once

#include <common.h>
#include <utils/commandLineOptions.h>
#include "core/gamestate.h"

#include <miniaudio.h>
#include <utils/mesh.h>
#include <utils/shader.h>
#include <utils/texture.h>
#include <utils/framebuffer.h>

#include <systems/systems.h>

#include <registry.h>
#include <window.h>



namespace df {
	class Application {
		public:
			// NOTE: You may want to use the constructor and destructor for initialization
			//       and deinitialization of objects. For the template we opted to use explicit
			//       initialization and deinitialization to avoid hidden control flow.
			static ::std::optional<Application> init(const CommandLineOptions& options) noexcept;
			void deinit() noexcept;
			void run() noexcept;

		private:
			Window* window;
			Registry* registry;
			// AudioSystem* audioEngine;

			WorldSystem world;
			// PhysicsSystem physics;
			RenderSystem render;

			void reset() noexcept;

			void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;
			void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) noexcept;
			void onScrollCallback(GLFWwindow* window, double xoffset, double yoffset) noexcept;
			void onResizeCallback(GLFWwindow* window, int width, int height) noexcept;

			// GameState
			GameState gameState;
	};
}
