#pragma once

#include <common.h>
#include <utils/command_line_options.h>

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
			static std::optional<Application> init(const CommandLineOptions& options) noexcept;
			void deinit() noexcept;
			void run() noexcept;

		private:
			Window* m_window;
			Registry* m_registry;
			AudioSystem* m_audio_engine;

			WorldSystem m_world;
			PhysicsSystem m_physics;
			RenderSystem m_render;

			void reset() noexcept;

			void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;
			void onResizeCallback(GLFWwindow* window, int width, int height) noexcept;

	};
}
