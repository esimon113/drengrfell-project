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
			count
		};


		template<typename AssetType>
		std::string getAssetPath(const AssetType assetId) noexcept;

	} // namespace assets
} // namespace df
