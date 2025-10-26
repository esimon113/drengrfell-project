#pragma once

#include <common.h>

namespace assets {

enum struct Sound {
	music,
	chicken_eat,
	chicken_dead,
	count
};

enum struct Mesh {
	chicken,
	count
};

enum struct Shader {
	chicken,
	sprite,
	wind,
	egg,
	count
};

enum struct Texture {
	eagle,
	bug,
	count
};

template<typename AssetType>
std::string getAssetPath(const AssetType asset_id) noexcept;

} // namespace assets
