#include "window.h"

std::optional<Window*> Window::init(const size_t width, const size_t height, const char* title) noexcept {
	Window* self = new Window;
	if (self == nullptr) {
		fmt::println(stderr, "Failed to allocate window");
		return std::nullopt;
	}

	self->m_window_extent = glm::uvec2(width, height);

	GLFWwindow* window = nullptr;
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#if defined(__APPLE__)
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#endif
	if (!(window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title,
		nullptr, nullptr))) {
		fmt::println(stderr, "Failed to create GLFW window");
		return std::nullopt;
	}
	self->m_handle = window;

	self->makeContextCurrent();

	glfwSetWindowUserPointer(self->handle(), self);
	glfwSetCursorPosCallback(self->handle(), Window::cursorPositionCallback);
	glfwSetKeyCallback(self->handle(), Window::keyCallback);
	glfwSetFramebufferSizeCallback(self->handle(), Window::resizeCallback);

	self->setKeyCallback([](GLFWwindow*, int, int, int, int) -> void {});
	self->setResizeCallback([](GLFWwindow*, int, int) -> void {});

	return self;
}

void Window::deinit() noexcept {
	glfwDestroyWindow(m_handle);
}

void Window::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) noexcept {
	Window* win = (Window*)glfwGetWindowUserPointer(window);
	win->m_cursor_position = glm::vec2(xpos, ypos);
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept {
	Window* win = (Window*)glfwGetWindowUserPointer(window);
	win->m_key_callback(window, key, scancode, action, mods);
}

void Window::resizeCallback(GLFWwindow* window, int width, int height) noexcept {
	Window* win = (Window*)glfwGetWindowUserPointer(window);
	win->m_window_extent = glm::uvec2(width, height);
	win->m_resize_callback(window, width, height);
}
