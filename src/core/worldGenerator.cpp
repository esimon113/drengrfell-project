#include "worldGenerator.h"
#include "perlinNoise.h"

namespace df {
    Result<std::vector<Tile>, ResultError> WorldGenerator::generateTiles(const WorldGeneratorConfig& config) noexcept {
        if (config.columns > 100) return Err(ResultError(ResultError::Kind::DomainError, "generateTiles: columns should not exceed 100"));
        if (config.rows > 100) return Err(ResultError(ResultError::Kind::DomainError, "generateTiles: rows should not exceed 100"));

        switch (config.generationMode) {
            case WorldGeneratorConfig::GenerationMode::INSULAR:
                return Ok(generateTilesInsular(config));
            default:
                return Ok(generateTilesPerlin(config));
        }
    }


    std::vector<Tile> WorldGenerator::generateTilesInsular(const WorldGeneratorConfig& config) noexcept {
        std::vector<Tile> tiles;

        const int columns = static_cast<int>(config.columns);
        const int rows = static_cast<int>(config.rows);

        auto randomEngine = std::default_random_engine(std::random_device()());
        auto uniformTileTypeDistribution = std::uniform_int_distribution(2, static_cast<int>(types::TileType::COUNT) - 1);

        // Only one ice-desert tile -> like in catan game
        std::unordered_map<int, int> tileCount;
        std::unordered_map<int, int> tileMax = {{ static_cast<int>(types::TileType::ICE),    1 }};

        for (int row = rows - 1; row >= 0; row--) {
            for (int column = 0; column < columns; column++) {
                // Creating an island with two water wide borders
                if(row<1 || column <1 || row > rows -2 || column > columns -2){
                    // make border tiles water
                    size_t id = row * columns + column;
                    tiles.emplace_back(id, types::TileType::WATER, types::TilePotency::MEDIUM);
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
                tiles.emplace_back(id, static_cast<types::TileType>(type), types::TilePotency::MEDIUM);
            }
        }
        return tiles;
    }

    std::vector<Tile> WorldGenerator::generateTilesPerlin(const WorldGeneratorConfig& config) noexcept {
        std::vector<Tile> tiles;

        const int columns = static_cast<int>(config.columns);
        const int rows = static_cast<int>(config.rows);

        const siv::PerlinNoise perlin{ config.seed };

        for (int row = 0; row < rows; row++) {
            for (int column = 0; column < columns; column++) {
                const double altitude = perlin.normalizedOctave2D_01(
                    static_cast<float>(row) * config.altitudeNoise.frequency,
                    static_cast<float>(column) * config.altitudeNoise.frequency,
                    static_cast<int>(config.altitudeNoise.octaves),
                    config.altitudeNoise.persistence
                );

                types::TileType type;
                if (altitude > 0.70f) {
                    type = types::TileType::MOUNTAIN;
                } else if (altitude > 0.50) {
                    type = types::TileType::GRASS;
                } else {
                    type = types::TileType::WATER;
                }

                size_t id = row * columns + column;
                tiles.emplace_back(id, type, types::TilePotency::MEDIUM);
            }
        }

        return tiles;
    }

}
