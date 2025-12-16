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
		"building-hover",
		"building-shadow",
		"hero",
		"menu"
	};


	template<> std::string assets::getAssetPath<assets::Shader>(const assets::Shader assetId) noexcept {
		return getBasePath() + "/assets/shaders/" + SHADER_FILES[static_cast<size_t>(assetId)];
	}


	static constexpr std::array<const char*, static_cast<size_t>(assets::Texture::count)> TEXTURE_FILES = {
		// add "*.png"-files that contain the textures here
		"tiles/tileAtlas.png",
		"settlements/viking-wood/viking-wood-settlement1.png",
		"roads/dirtRoad/dirt_road_diagonal_up.png",
		"mainMenu/temp_menu_background.png",
		"mainMenu/temp_menu_title.png",
		"mainMenu/temp_menu_start.png",
		"mainMenu/temp_menu_exit.png"
	};


	template<> std::string assets::getAssetPath<assets::Texture>(const assets::Texture assetId) noexcept {
		return getBasePath() + "/assets/textures/" + TEXTURE_FILES[static_cast<size_t>(assetId)];
	}

	static constexpr std::array<const char*, static_cast<size_t>(assets::MenuTexture::count)> MENU_TEXTURE_FILES = {
		"configMenu/temp_configMenu_ai.png",
		"configMenu/temp_configMenu_background.png",
		"configMenu/temp_configMenu_easy.png",
		"configMenu/temp_configMenu_hard.png",
		"configMenu/temp_configMenu_height.png",
		"configMenu/temp_configMenu_insular.png",
		"configMenu/temp_configMenu_medium.png",
		"configMenu/temp_configMenu_multiplayer.png",
		"configMenu/temp_configMenu_perlin.png",
		"configMenu/temp_configMenu_seed.png",
		"configMenu/temp_configMenu_start.png",
		"configMenu/temp_configMenu_title.png",
		"configMenu/temp_configMenu_width.png"
	};


	template<> std::string assets::getAssetPath<assets::MenuTexture>(const assets::MenuTexture assetId) noexcept {
		return getBasePath() + "/assets/textures/" + MENU_TEXTURE_FILES[static_cast<size_t>(assetId)];
	}
}
