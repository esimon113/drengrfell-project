#include "assets.h"



namespace df {
	static constexpr std::array<const char*, static_cast<size_t>(assets::Sound::count)> SOUND_FILES = {
		"music.wav",
		"chicken_eat.wav",
		"chicken_dead.wav",
	};


	template<> std::string assets::getAssetPath<assets::Sound>(const assets::Sound asset_id) noexcept {
		return getBasePath() + "/assets/sounds/" + SOUND_FILES[static_cast<size_t>(asset_id)];
	}


	static constexpr std::array<const char*, static_cast<size_t>(assets::Mesh::count)> MESH_FILES = {
		"chicken.obj",
	};


	template<> std::string assets::getAssetPath<assets::Mesh>(const assets::Mesh asset_id) noexcept {
		return getBasePath() + "/assets/mesh/" + MESH_FILES[static_cast<size_t>(asset_id)];
	}


	static constexpr std::array<const char*, static_cast<size_t>(assets::Shader::count)> SHADER_FILES = {
		"chicken",
		"sprite",
		"wind",
		"egg",
	};


	template<> std::string assets::getAssetPath<assets::Shader>(const assets::Shader asset_id) noexcept {
		return getBasePath() + "/assets/shaders/" + SHADER_FILES[static_cast<size_t>(asset_id)];
	}


	static constexpr std::array<const char*, static_cast<size_t>(assets::Texture::count)> TEXTURE_FILES = {
		"eagle.png",
		"bug.png",
	};


	template<> std::string assets::getAssetPath<assets::Texture>(const assets::Texture asset_id) noexcept {
		return getBasePath() + "/assets/textures/" + TEXTURE_FILES[static_cast<size_t>(asset_id)];
	}
}
