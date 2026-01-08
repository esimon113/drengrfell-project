#include "window.h"
#include "GLFW/glfw3.h"
#include <memory>



namespace df {
	std::unique_ptr<Window> Window::init(const size_t width, const size_t height, const char* title) noexcept {
		auto self = std::make_unique<Window>();

		if (!self) {
			fmt::println(stderr, "Failed to allocate window");

			return nullptr;
		}

		self->windowExtent = glm::uvec2(width, height);

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

		window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title, nullptr, nullptr);
		// if (!(window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title,
		// nullptr, nullptr))) {
		if (!window) {
			fmt::println(stderr, "Failed to create GLFW window");
			return nullptr;
		}
		self->handle = window;

		self->makeContextCurrent();

		int fbWidth, fbHeight;
		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
		self->windowExtent = glm::uvec2(fbWidth, fbHeight);

		glfwSetWindowUserPointer(self->getHandle(), self.get());
		glfwSetCursorPosCallback(self->getHandle(), Window::cursorPositionCallback);
		glfwSetKeyCallback(self->getHandle(), Window::keyCallback);
		glfwSetMouseButtonCallback(self->getHandle(), Window::mouseButtonCallback);
		glfwSetScrollCallback(self->getHandle(), Window::scrollCallback);
		glfwSetFramebufferSizeCallback(self->getHandle(), Window::resizeCallback);

		self->setKeyCallback([](GLFWwindow*, int, int, int, int) -> void {});
		self->setMouseButtonCallback([](GLFWwindow*, int, int, int) {});
		self->setScrollCallback([](GLFWwindow*, double, double) {});
		self->setResizeCallback([](GLFWwindow*, int, int) -> void {});

		return self;
	}


	Window::~Window() noexcept {
		if (handle) {
			// Unset the current context before destroying the window
			// This is important for proper cleanup, especially on Wayland
			glfwMakeContextCurrent(nullptr);
			glfwDestroyWindow(handle);
			handle = nullptr;
		}
	}

	void Window::deinit() noexcept {
		if (handle) {
			// Unset the current context before destroying the window
			// This is important for proper cleanup, especially on Wayland
			glfwMakeContextCurrent(nullptr);
			glfwDestroyWindow(handle);
			handle = nullptr;
		}
	}


	void Window::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) noexcept {
		Window* win = (Window*)glfwGetWindowUserPointer(window);

		int winWidth, winHeight;
		glfwGetWindowSize(window, &winWidth, &winHeight);

		int fbWidth, fbHeight;
		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

		// Calculate scale ratio manually
		float scaleX = (float)fbWidth / (float)winWidth;
		float scaleY = (float)fbHeight / (float)winHeight;

		win->cursorPosition = glm::vec2(xpos * scaleX, ypos * scaleY);
	}


	void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept {
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		win->keyFunction(window, key, scancode, action, mods);
	}

	void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) noexcept {
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		win->mouseButtonFunction(window, button, action, mods);
	}

	void Window::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) noexcept {
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		win->scrollFunction(window, xoffset, yoffset);
	}


	void Window::resizeCallback(GLFWwindow* window, int width, int height) noexcept {
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		win->windowExtent = glm::uvec2(width, height);
		win->resizeFunction(window, width, height);
	}
} // namespace df
