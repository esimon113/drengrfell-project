#include "worldGenerator.h"
#include "perlinNoise.h"

namespace df {
    Result<std::vector<Tile>, ResultError> WorldGenerator::generateTiles(WorldGeneratorConfig config) noexcept {
        if (config.columns > 100) return Err(ResultError(ResultError::Kind::DomainError, "generateTiles: columns should not exceed 100"));
        if (config.rows > 100) return Err(ResultError(ResultError::Kind::DomainError, "generateTiles: rows should not exceed 100"));
        if (config.seed == 0) {
            auto randomEngine = std::default_random_engine(std::random_device()());
            config.seed = std::uniform_int_distribution()(randomEngine);
        }


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


    enum class WhittakerBiome {
        TUNDRA,
        BOREAL_FOREST,
        TEMPERATE_GRASSLAND,
        SHRUBLAND,
        TEMPERATE_SEASONAL_FOREST,
        TEMPERATE_RAINFOREST,
        SUBTROPICAL_DESERT,
        SAVANNA,
        TROPICAL_RAINFOREST
    };


    WhittakerBiome calculateBiome(const double temperature, const double precipitation) noexcept {
        // Biomes are based on https://commons.wikimedia.org/wiki/File:Climate_influence_on_terrestrial_biome.svg

        const double celsius = temperature * 43.0 - 10.0;
        const double precipCm = precipitation * 400.0;

        if (celsius > 20) {
            if (precipCm > 250) {
                return WhittakerBiome::TROPICAL_RAINFOREST;
            } else if (precipCm > 100) {
                return WhittakerBiome::SAVANNA;
            } else {
                return WhittakerBiome::SUBTROPICAL_DESERT;
            }
        } else if (celsius > 10) {
            if (precipCm > 200) {
                return WhittakerBiome::TEMPERATE_RAINFOREST;
            } else if (precipCm > 100) {
                return WhittakerBiome::TEMPERATE_SEASONAL_FOREST;
            } else if (precipCm > 50) {
                return WhittakerBiome::SHRUBLAND;
            } else {
                return WhittakerBiome::TEMPERATE_GRASSLAND;
            }
        } else if (celsius > 0) {
            return WhittakerBiome::BOREAL_FOREST;;
        } else {
            return WhittakerBiome::TUNDRA;
        }
    }


    std::vector<Tile> WorldGenerator::generateTilesPerlin(const WorldGeneratorConfig& config) noexcept {
        std::vector<Tile> tiles;

        const int columns = static_cast<int>(config.columns);
        const int rows = static_cast<int>(config.rows);

        auto randomEngine = std::default_random_engine(std::random_device()());
        auto uniformTileTypeDistribution = std::uniform_int_distribution(2, static_cast<int>(types::TileType::COUNT) - 1);

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
                if (config.useWhittakerBiomes) {
                    // This uses Whittakers simplification of Holdridge's life zones.
                    // See https://en.wikipedia.org/wiki/Holdridge_life_zones
                    // and https://en.wikipedia.org/wiki/Biome#Whittaker_(1962,_1970,_1975)_biome-types
                    // and https://commons.wikimedia.org/wiki/File:Climate_influence_on_terrestrial_biome.svg

                    const double latitude = std::abs((row / static_cast<double>(rows)) * 180.0 - 90.0);
                    const double equatorProximity = (90.0 - latitude) / 90.0;
                    const double temperature = perlin.normalizedOctave2D_01(
                        static_cast<float>(row) * config.temperatureNoise.frequency,
                        static_cast<float>(column) * config.temperatureNoise.frequency,
                        static_cast<int>(config.temperatureNoise.octaves),
                        config.temperatureNoise.persistence
                    ) * equatorProximity;
                    double precipitation = perlin.normalizedOctave2D_01(
                        static_cast<float>(row) * config.precipitationNoise.frequency,
                        static_cast<float>(column) * config.precipitationNoise.frequency,
                        static_cast<int>(config.precipitationNoise.octaves),
                        config.precipitationNoise.persistence
                    );

                    precipitation *= temperature;

                    if (altitude > 0.60f) {
                        type = types::TileType::MOUNTAIN;
                    } else if (altitude > 0.42) {
                        auto biome = calculateBiome(temperature, precipitation);
                        switch (biome) {
                            case WhittakerBiome::TUNDRA:
                                type = types::TileType::GRASS;
                                break;
                            case WhittakerBiome::BOREAL_FOREST:
                                type = types::TileType::FOREST;
                                break;
                            case WhittakerBiome::TEMPERATE_GRASSLAND:
                                type = types::TileType::GRASS;
                                break;
                            case WhittakerBiome::SHRUBLAND:
                                type = types::TileType::GRASS;
                                break;
                            case WhittakerBiome::TEMPERATE_SEASONAL_FOREST:
                                type = types::TileType::FOREST;
                                break;
                            case WhittakerBiome::TEMPERATE_RAINFOREST:
                                type = types::TileType::FOREST;
                                break;
                            case WhittakerBiome::SUBTROPICAL_DESERT:
                                type = types::TileType::CLAY;
                                break;
                            case WhittakerBiome::SAVANNA:
                                type = types::TileType::FIELD;
                                break;
                            case WhittakerBiome::TROPICAL_RAINFOREST:
                                type = types::TileType::FOREST;
                                break;
                        }
                    } else {
                        if ((temperature * 43.0 - 10.0) < 0) {
                            type = types::TileType::ICE;
                        } else {
                            type = types::TileType::WATER;
                        }
                    }
                } else {
                    // TODO: Add altitudes to world generation configuration
                    // TODO: Add variation chances to world generation configuration
                    if (altitude > 0.60f) {
                        type = types::TileType::MOUNTAIN;
                    } else if (altitude > 0.58) {
                        type = types::TileType::FOREST;
                    } else if (altitude > 0.42) {
                        type = static_cast<types::TileType>(uniformTileTypeDistribution(randomEngine));
                    } else {
                        type = types::TileType::WATER;
                    }
                }

                size_t id = row * columns + column;
                tiles.emplace_back(id, type, types::TilePotency::MEDIUM);
            }
        }

        return tiles;
    }

}
