#pragma once

#include <common.h>



namespace df {
	namespace assets {
		enum struct Sound {
			music,
			count
		};


		enum struct Mesh {
			count
		};

		// ORDER IS IMPORTANT
		enum struct Shader {
			sprite,
			wind,
			tile,
			buildingHover,
			buildingShadow,
			hero,
			menu,
			particle,
			text,
			hud,
			count
		};


		enum struct Texture {
			TILE_ATLAS,
			VIKING_WOOD_SETTLEMENT1,
			DIRT_ROAD_DIAGONAL_UP,
			MENU_BACKGROUND,
			MENU_TITLE,
			MENU_START,
			MENU_EXIT,

			// hero

			HERO_IDLE_0,
			HERO_IDLE_1,
			HERO_IDLE_2,

			HERO_SWIM_0,
			HERO_SWIM_1,
			HERO_SWIM_2,
			HERO_SWIM_3,
			HERO_SWIM_4,
			HERO_SWIM_5,

			HERO_JUMP_0,
			HERO_JUMP_1,
			HERO_JUMP_2,
			HERO_JUMP_3,
			HERO_JUMP_4,
			HERO_JUMP_5,

			HERO_ATTACK_0,
			HERO_ATTACK_1,

			HERO_RUN_0,
			HERO_RUN_1,
			HERO_RUN_2,
			HERO_RUN_3,
			HERO_RUN_4,
			HERO_RUN_5,
			count
		};

		enum struct JsonFile {
			WORLD_GENERATION_CONFIGURATION,
			COUNT,
		};

		enum struct MenuTexture {
			CONFIG_AI,
			CONFIG_BACKGROUND,
			CONFIG_EASY,
			CONFIG_HARD,
			CONFIG_HEIGHT,
			CONFIG_INSULAR,
			CONFIG_MEDIUM,
			CONFIG_MULTIPLAYER,
			CONFIG_PERLIN,
			CONFIG_SEED,
			CONFIG_START,
			CONFIG_TITLE,
			CONFIG_WIDTH,
			count
		};


		template<typename AssetType>
		std::string getAssetPath(const AssetType assetId) noexcept;

	} // namespace assets
} // namespace df
