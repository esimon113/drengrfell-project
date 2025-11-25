cmake_minimum_required(VERSION 3.24)
include(FetchContent)

# propagate variables set in this project to fetched dependencies
cmake_policy(SET CMP0077 NEW)

project(drengrfell VERSION 0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the build type to Debug if no other build type was selected.
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

# Fetch some basic dependencies for creating windows (glfw), vector math (glm),
# audio (miniaudio) and loading OpenGL profiles (gl3w)
FetchContent_Declare(
	glfw # Licensed under Zlib
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG 3.4
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_Declare(
	glm # Licensed under MIT (or the Happy Bunny License [modified MIT])
	GIT_REPOSITORY https://github.com/g-truc/glm.git
	GIT_TAG 1.0.1
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_Declare(
	miniaudio # Public Domain (or licensed under MIT)
	GIT_REPOSITORY https://github.com/mackron/miniaudio.git
	GIT_TAG 0.11.22
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_Declare(
	gl3w # Public Domain
	GIT_REPOSITORY https://github.com/skaslev/gl3w.git
	GIT_TAG        master
	GIT_SHALLOW    TRUE
	GIT_PROGRESS   TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_Declare(
	tinyECS # Licensed under MIT
	GIT_REPOSITORY https://github.com/hrhodin/tinyECS.git
	GIT_TAG        main
	GIT_SHALLOW    TRUE
	GIT_PROGRESS   TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(glfw glm miniaudio gl3w tinyECS)

# You can fetch additional libraries here, e.g. `fmt`
FetchContent_Declare(
	fmt # Licensed under MIT
	GIT_REPOSITORY https://github.com/fmtlib/fmt.git
	GIT_TAG 11.2.0
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_Declare(
	tinyobj # Licensed under MIT
	GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
	GIT_TAG release
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_Declare(
	stb # Public Domain
	GIT_REPOSITORY https://github.com/nothings/stb.git
	GIT_TAG master
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_Declare(
				nlohmann_json # MIT Licence
				GIT_REPOSITORY https://github.com/nlohmann/json.git
				GIT_TAG v3.12.0
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(
	fmt
	tinyobj
	stb
	nlohmann_json
)

# Configure compiler flags as an interface library
add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_20)
target_compile_definitions(compiler_flags INTERFACE $<$<CONFIG:Debug>:DEBUG>)

if (MSVC)
	target_compile_definitions(compiler_flags INTERFACE _USE_MATH_DEFINES)
	target_compile_options(compiler_flags INTERFACE /W4 /wd4244 /wd4267 /wd4838)
else()
	target_compile_options(compiler_flags INTERFACE -Wall -Wextra -Wpedantic)
endif()

# Configure header containing project metadata
configure_file(${PROJECT_SOURCE_DIR}/src/project_config.h.in project_config.h)

add_executable(${PROJECT_NAME}
	${PROJECT_SOURCE_DIR}/src/main.cpp
	${PROJECT_SOURCE_DIR}/src/common.cpp
	${PROJECT_SOURCE_DIR}/src/assets.cpp
	${PROJECT_SOURCE_DIR}/src/application.cpp
	${PROJECT_SOURCE_DIR}/src/registry.cpp
	${PROJECT_SOURCE_DIR}/src/window.cpp

	${PROJECT_SOURCE_DIR}/src/core/graph.cpp
	${PROJECT_SOURCE_DIR}/src/core/tile.cpp
	${PROJECT_SOURCE_DIR}/src/core/edge.cpp
	${PROJECT_SOURCE_DIR}/src/core/vertex.cpp
	${PROJECT_SOURCE_DIR}/src/core/settlement.cpp
	${PROJECT_SOURCE_DIR}/src/core/road.cpp

	${PROJECT_SOURCE_DIR}/src/systems/world.cpp
	${PROJECT_SOURCE_DIR}/src/systems/physics.cpp
	${PROJECT_SOURCE_DIR}/src/systems/render.cpp
	${PROJECT_SOURCE_DIR}/src/systems/audio.cpp

	${PROJECT_SOURCE_DIR}/src/utils/mesh.cpp
	${PROJECT_SOURCE_DIR}/src/utils/shader.cpp
	${PROJECT_SOURCE_DIR}/src/utils/framebuffer.cpp
	${PROJECT_SOURCE_DIR}/src/utils/texture.cpp
 "src/core/hero.cpp" "src/utils/animations.cpp" "src/core/hero.h" "src/utils/animations.h" "src/core/components.h" "src/systems/animationSystem.h" "src/systems/animationSystem.cpp")

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON
	EXPORT_COMPILE_COMMANDS ON
)

target_include_directories(${PROJECT_NAME} PUBLIC
	${PROJECT_BINARY_DIR} # location of the configured header
	${PROJECT_SOURCE_DIR}/src # location of our own headers
	${PROJECT_SOURCE_DIR}/src/core # location of core
	${PROJECT_SOURCE_DIR}/src/systems # location of systems
	${PROJECT_SOURCE_DIR}/src/utils # location of utils
	${stb_SOURCE_DIR} # location of the `stb_image` header
)

target_link_libraries(${PROJECT_NAME} PUBLIC
	compiler_flags
	glfw
	glm::glm
	miniaudio
	tinyobjloader
	gl3w
	fmt
	tinyECS
	nlohmann_json::nlohmann_json
)
