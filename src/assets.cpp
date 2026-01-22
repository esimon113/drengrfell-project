#include "assets.h"



namespace df {
	static constexpr std::array<const char*, static_cast<size_t>(assets::Sound::count)> SOUND_FILES = {
		"background/Fires Beneath the Trees.mp3",
		"background/Foundations of Stone.mp3",
		"background/Rest of the Hearthlands.mp3",
		"background/The Weaver's Hope.mp3",
	};


	template <>
	std::string assets::getAssetPath<assets::Sound>(const assets::Sound assetId) noexcept {
		return getBasePath() + "/assets/sounds/" + SOUND_FILES[static_cast<size_t>(assetId)];
	}


	static constexpr std::array<const char*, static_cast<size_t>(assets::Mesh::count)> MESH_FILES = {
		// add "*.obj"-files here
	};


	template <>
	std::string assets::getAssetPath<assets::Mesh>(const assets::Mesh assetId) noexcept {
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
		"hud",
		"tilepick",
		"location-highlight"};


	template <>
	std::string assets::getAssetPath<assets::Shader>(const assets::Shader assetId) noexcept {
		return getBasePath() + "/assets/shaders/" + SHADER_FILES[static_cast<size_t>(assetId)];
	}


	static constexpr std::array<const char*, static_cast<size_t>(assets::Texture::count)> TEXTURE_FILES = {
		// add "*.png"-files that contain the textures here
		"tiles/tileAtlas1.png",
		"tiles/tileAtlas2.png",
		"settlements/viking-wood/viking-wood-settlement1.png",
		"settlements/viking-wood/viking-wood-settlement2.png",
		"settlements/viking-wood/viking-wood-settlement3.png",
		"settlements/viking-wood/viking-wood-settlement4.png",
		"settlements/viking-wood/viking-wood-settlement5.png",
		"settlements/stone-settlement/stone-settlement1.png",
		"settlements/stone-settlement/stone-settlement2.png",
		"settlements/stone-settlement/stone-settlement3.png",
		"settlements/stone-settlement/stone-settlement4.png",
		"settlements/stone-settlement/stone-settlement5.png",
		"settlements/stone-settlement/stone-settlement6.png",
		"roads/dirtRoad/dirt_road_diagonal_up.png",
		"roads/dirtRoad/dirt_road_diagonal_down.png",
		"roads/dirtRoad/dirt_road_vertical.png",
		"roads/pathRoad/path_road_vertical.png",
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

		"hero/run/run_0.png",
		"hero/run/run_1.png",
		"hero/run/run_2.png",
		"hero/run/run_3.png",
		"hero/run/run_4.png",
		"hero/run/run_5.png",

		"ressources/tmp_wood_frame.png",

		"settlements/castle/castle-sprites1.png",
		"settlements/castle/castle-sprites2.png",
		"settlements/castle/castle-sprites3.png",
		"settlements/castle/castle-sprites4.png",
		"settlements/castle/castle-sprites5.png",
		"settlements/castle/castle-sprites6.png",
		"settlements/castle/castle-sprites7.png",
		"settlements/castle/castle-sprites8.png",

		"settlements/productivity-boost/lumber-camp.png",
		"settlements/productivity-boost/stone-quarry.png",
		"settlements/productivity-boost/stable.png",
		"settlements/productivity-boost/mill.png",
		"settlements/productivity-boost/brick-kiln.png",
	};


	template <>
	std::string assets::getAssetPath<assets::Texture>(const assets::Texture assetId) noexcept {
		return getBasePath() + "/assets/textures/" + TEXTURE_FILES[static_cast<size_t>(assetId)];
	}


	static constexpr std::array<const char*, static_cast<size_t>(assets::JsonFile::COUNT)> JSON_FILES = {
		"world_generation.json",
	};


	template <>
	std::string assets::getAssetPath<assets::JsonFile>(const assets::JsonFile assetId) noexcept {
		return getBasePath() + "/assets/jsons/" + JSON_FILES[static_cast<size_t>(assetId)];
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
		"configMenu/temp_configMenu_width.png"};


	template <>
	std::string assets::getAssetPath<assets::MenuTexture>(const assets::MenuTexture assetId) noexcept {
		return getBasePath() + "/assets/textures/" + MENU_TEXTURE_FILES[static_cast<size_t>(assetId)];
	}
} // namespace df
