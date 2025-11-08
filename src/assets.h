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
			count
		};


		enum struct Texture {
			count
		};


		template<typename AssetType>
		std::string getAssetPath(const AssetType assetId) noexcept;

	} // namespace assets
} // namespace df
