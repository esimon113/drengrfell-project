#include "assets.h"



namespace df {
	static constexpr std::array<const char*, static_cast<size_t>(assets::Sound::count)> SOUND_FILES = {
		"music.wav",
	};


	template<> std::string assets::getAssetPath<assets::Sound>(const assets::Sound assetId) noexcept {
		return getBasePath() + "/assets/sounds/" + SOUND_FILES[static_cast<size_t>(assetId)];
	}


	static constexpr std::array<const char*, static_cast<size_t>(assets::Mesh::count)> MESH_FILES = {
		// add "*.obj"-files here
	};


	template<> std::string assets::getAssetPath<assets::Mesh>(const assets::Mesh assetId) noexcept {
		return getBasePath() + "/assets/mesh/" + MESH_FILES[static_cast<size_t>(assetId)];
	}


	static constexpr std::array<const char*, static_cast<size_t>(assets::Shader::count)> SHADER_FILES = {
		"sprite",
		"wind",
		"tile",
		"settlement-hover",
		"settlement-shadow"
	};


	template<> std::string assets::getAssetPath<assets::Shader>(const assets::Shader assetId) noexcept {
		return getBasePath() + "/assets/shaders/" + SHADER_FILES[static_cast<size_t>(assetId)];
	}


	static constexpr std::array<const char*, static_cast<size_t>(assets::Texture::count)> TEXTURE_FILES = {
		// add "*.png"-files that contain the textures here
		"tiles/tileAtlas.png",
		"settlements/viking-wood/viking-wood-settlement1.png"
	};


	template<> std::string assets::getAssetPath<assets::Texture>(const assets::Texture assetId) noexcept {
		return getBasePath() + "/assets/textures/" + TEXTURE_FILES[static_cast<size_t>(assetId)];
	}
}
