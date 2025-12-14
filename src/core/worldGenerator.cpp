#include "worldGenerator.h"

namespace df {

    Result<std::vector<Tile>, ResultError> WorldGenerator::generateTiles(const WorldGeneratorConfig& config) noexcept {
        if (config.columns > 100) return Err(ResultError(ResultError::Kind::DomainError, "generateTiles: columns should not exceed 100"));
        if (config.rows > 100) return Err(ResultError(ResultError::Kind::DomainError, "generateTiles: rows should not exceed 100"));
        const int columns = static_cast<int>(config.columns);
        const int rows = static_cast<int>(config.rows);

        auto randomEngine = std::default_random_engine(std::random_device()());
        auto uniformTileTypeDistribution = std::uniform_int_distribution(2, static_cast<int>(types::TileType::COUNT) - 1);

        std::vector<Tile> tiles;

        // Only one ice-desert tile -> like in catan game
        std::unordered_map<int, int> tileCount;
        std::unordered_map<int, int> tileMax = {{ static_cast<int>(types::TileType::ICE),    1 }};

        for (int row = rows - 1; row >= 0; row--) {
            for (int column = 0; column < columns; column++) {
                // Creating an island with two water wide borders
                if(row<1 || column <1 || row > rows -2 || column > columns -2){
                    // make border tiles water
                    size_t id = row * columns + column;
                    tiles.push_back({id, types::TileType::WATER, types::TilePotency::MEDIUM});
                    continue;
                }
                int type = uniformTileTypeDistribution(randomEngine);

                if(tileMax.contains(type)){
                    if(tileCount[type] >= tileMax[type]){
                        do {
                            type = uniformTileTypeDistribution(randomEngine); // TODO: look for a more optimal solution
                        } while (type == static_cast<int>(types::TileType::ICE));
                    } else {
                        tileCount[type]++;
                    }
                }

                size_t id = row * columns + column;
                tiles.push_back({id, static_cast<types::TileType>(type), types::TilePotency::MEDIUM});
            }
        }
        return Ok(tiles);
    }

}
