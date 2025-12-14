#pragma once
#include "tile.h"
#include "resultError.h"

namespace df {
    class WorldGenerator final {
    public:
        WorldGenerator() = default;

        static Result<std::vector<Tile>, ResultError> generateTiles(unsigned columns = 10.0f, unsigned rows = 10.0f) noexcept;
    };
}
