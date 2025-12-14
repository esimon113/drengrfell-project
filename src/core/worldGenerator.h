#pragma once
#include "tile.h"
#include "resultError.h"
#include "worldGeneratorConfig.h"

namespace df {
    class WorldGenerator final {
    public:
        WorldGenerator() = default;

        static Result<std::vector<Tile>, ResultError> generateTiles(const WorldGeneratorConfig& config) noexcept;
    };
}
