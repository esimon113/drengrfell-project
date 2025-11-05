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

			inline GLFWwindow* handle() noexcept { return m_handle; }
			inline void makeContextCurrent() noexcept { glfwMakeContextCurrent(m_handle); }
			inline void unsetCurrentContext() noexcept { glfwMakeContextCurrent(nullptr); }
			inline bool shouldClose() noexcept { return glfwWindowShouldClose(m_handle); }
			inline void close() noexcept { glfwSetWindowShouldClose(m_handle, GLFW_TRUE); }
			inline void swapBuffers() noexcept { glfwSwapBuffers(m_handle); }
			inline void setTitle(const char* title) noexcept { glfwSetWindowTitle(m_handle, title); }

			inline void setKeyCallback(KeyFunction key_function) noexcept { m_key_callback = key_function; }
			inline void setResizeCallback(ResizeFunction resize_function) noexcept { m_resize_callback = resize_function; }

			inline glm::vec2 cursorPosition() const noexcept { return m_cursor_position; }
			inline glm::uvec2 windowExtent() const noexcept { return m_window_extent; }
			
			inline glm::vec2 contentScale() const noexcept {
				glm::vec2 scale;
				glfwGetWindowContentScale(m_handle, &scale.x, &scale.y);
				return scale;
			}


		private:
			GLFWwindow* m_handle;

			glm::uvec2 m_window_extent;
			ResizeFunction m_resize_callback;
			static void resizeCallback(GLFWwindow*, int width, int height) noexcept;

			glm::vec2 m_cursor_position;
			static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) noexcept;

			KeyFunction m_key_callback;
			static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;
	};
} // namespace df
