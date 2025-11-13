#pragma once

#include <common.h>



namespace df {
	class Window {
		public:
			using ResizeFunction = std::function<void(GLFWwindow*, int, int)>;
			using KeyFunction = std::function<void(GLFWwindow*, int, int, int, int)>;

			Window() = default;
			~Window() = default;

			static std::optional<Window*> init(const size_t width, const size_t height, const char* title) noexcept;
			void deinit() noexcept;

			inline GLFWwindow* getHandle() noexcept { return handle; }
			inline void makeContextCurrent() noexcept { glfwMakeContextCurrent(handle); }
			inline void unsetCurrentContext() noexcept { glfwMakeContextCurrent(nullptr); }
			inline bool shouldClose() noexcept { return glfwWindowShouldClose(handle); }
			inline void close() noexcept { glfwSetWindowShouldClose(handle, GLFW_TRUE); }
			inline void swapBuffers() noexcept { glfwSwapBuffers(handle); }
			inline void setTitle(const char* title) noexcept { glfwSetWindowTitle(handle, title); }

			inline void setKeyCallback(KeyFunction keyFunc) noexcept { this->keyFunction = keyFunc; }
			inline void setResizeCallback(ResizeFunction resizeFunc) noexcept { this->resizeFunction = resizeFunc; }

			inline glm::vec2 getCursorPosition() const noexcept { return cursorPosition; }
			inline glm::uvec2 getWindowExtent() const noexcept { return windowExtent; }


			inline glm::vec2 getContentScale() const noexcept {
				glm::vec2 scale;
				glfwGetWindowContentScale(handle, &scale.x, &scale.y);

				return scale;
			}


		private:
			GLFWwindow* handle;

			glm::uvec2 windowExtent;
			ResizeFunction resizeFunction;
			static void resizeCallback(GLFWwindow*, int width, int height) noexcept;

			glm::vec2 cursorPosition;
			static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) noexcept;

			KeyFunction keyFunction;
			static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;
	};
} // namespace df
