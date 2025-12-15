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

	// ORDER IS IMPORTANT
	static constexpr std::array<const char*, static_cast<size_t>(assets::Shader::count)> SHADER_FILES = {
		"sprite",
		"wind",
		"tile",
		"building-hover",
		"building-shadow",
		"hero",
		"menu",
		"particle",
		"text",
		"hud"
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
		"mainMenu/temp_menu_exit.png",
		"hero/idle/idle_0.png",
		"hero/idle/idle_1.png",
		"hero/idle/idle_2.png",
	
		"hero/swim/swim_0.png",
		"hero/swim/swim_1.png",
		"hero/swim/swim_2.png",
		"hero/swim/swim_3.png",
		"hero/swim/swim_4.png",
		"hero/swim/swim_5.png",

		"hero/jump/jump_0.png",
		"hero/jump/jump_1.png",
		"hero/jump/jump_2.png",
		"hero/jump/jump_3.png",
		"hero/jump/jump_4.png",
		"hero/jump/jump_5.png",

		"hero/attack/attack_0.png",
		"hero/attack/attack_1.png",
	};


	template<> std::string assets::getAssetPath<assets::Texture>(const assets::Texture assetId) noexcept {
		return getBasePath() + "/assets/textures/" + TEXTURE_FILES[static_cast<size_t>(assetId)];
	}
}
