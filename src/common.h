#pragma once

// include project configuration, version, etc.
#include <project_config.h>

// add additional commonly used headers here
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fmt/base.h>
#include <fmt/format.h>
#include <functional>
#include <map>
#include <optional>
#include <random>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>


#if defined(_WIN32) && defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif

#include <GL/gl3w.h>

#if defined(_WIN32) && defined(__clang__)
#pragma clang diagnostic pop
#endif


#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <stb_image.h>



namespace df {
	std::string getBasePath() noexcept;
} // namespace df
