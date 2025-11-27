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


		enum struct Shader {
			sprite,
			wind,
			tile,
			buildingHover,
			buildingShadow,
			hero,
			count
		};


		enum struct Texture {
			TILE_ATLAS,
			VIKING_WOOD_SETTLEMENT1,
			DIRT_ROAD_DIAGONAL_UP,
			count
		};


		template<typename AssetType>
		std::string getAssetPath(const AssetType assetId) noexcept;

	} // namespace assets
} // namespace df
