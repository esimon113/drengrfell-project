#include "common.h"
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if defined(_WIN32)
#include <windows.h>
#endif

std::string getBasePath() noexcept {
#if defined(PACKAGE)
	std::filesystem::path binpath;
#if defined(__linux__)
	binpath = std::filesystem::read_symlink("/proc/self/exe");
#elif defined(_WIN32)
	char buf[512] = {0}; // this should be plenty...
	size_t buflen = 512;
	GetModuleFileNameA(NULL, buf, buflen);
	binpath = std::filesystem::path(buf);
#elif defined(__APPLE__)
#error Not implemented yet
#endif

	return std::filesystem::path(binpath).parent_path().string();
#else
	return BASE_PATH;
#endif
}
