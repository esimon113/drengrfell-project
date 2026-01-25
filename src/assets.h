#pragma once

#include <common.h>



namespace df {
	namespace assets {
		enum struct Sound {
			BACKGROUND_MUSIC_1,
			BACKGROUND_MUSIC_2,
			BACKGROUND_MUSIC_3,
			BACKGROUND_MUSIC_4,
			EVENT_POPUP,
			EVENT_BEAR,
			EVENT_BLIZZARD,
			EVENT_MUD,
			EVENT_ROCKSLIDE,
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
			tilePicker,
			locationHighlight,
			dimScreen,
			count
		};


		enum struct Texture {
			TILE_ATLAS,
			TILE_ATLAS2,
			VIKING_WOOD_SETTLEMENT1,
			VIKING_WOOD_SETTLEMENT2,
			VIKING_WOOD_SETTLEMENT3,
			VIKING_WOOD_SETTLEMENT4,
			VIKING_WOOD_SETTLEMENT5,
			STONE_SETTLEMENT1,
			STONE_SETTLEMENT2,
			STONE_SETTLEMENT3,
			STONE_SETTLEMENT4,
			STONE_SETTLEMENT5,
			STONE_SETTLEMENT6,
			DIRT_ROAD_DIAGONAL_UP,
			DIRT_ROAD_DIAGONAL_DOWN,
			DIRT_ROAD_VERTICAL,
			PATH_ROAD_VERTICAL,
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

			HAZARD_BEAR,
			HAZARD_BLIZZARD,
			HAZARD_MUD,
			HAZARD_ROCKSLIDE,
			
			CASTLE1,
			CASTLE2,
			CASTLE3,
			CASTLE4,
			CASTLE5,
			CASTLE6,
			CASTLE7,
			CASTLE8,

			LUMBER_CAMP,
			STONE_QUARRY,
			STABLE,
			MILL,
			BRICK_KILN,
			PRODUCTIVITY_PLACEHOLDER,

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


		template <typename AssetType>
		std::string getAssetPath(const AssetType assetId) noexcept;

	} // namespace assets
} // namespace df
