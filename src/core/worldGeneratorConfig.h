#pragma once
#include <nlohmann/json.hpp>

namespace df {
    // This is intended to be used like a plain old data structure with serialization. Nothing more.
    // Because of the implementation of deserialize, everything after = is used as a default value,
    // if the property or its value (i.e. null) is not present in the JSON.
    struct WorldGeneratorConfig {
        // Meta
        unsigned version = 1; // For backwards/forwards compatibility

        // Generator independent
        unsigned columns = 24; // Map width
        unsigned rows = 24; // Map height
        enum class GenerationMode {
            INSULAR, // Like in the classic board game
            PERLIN,  // TODO: Find better name
        };
        GenerationMode generationMode = GenerationMode::PERLIN;

        // General
        unsigned seed = 123; // The same seed creates the same world
        // Noise
        struct AltitudeNoiseConfig {
            float frequency = 0.1f; // Same meaning as in sine waves
            float persistence = 0.50f; // The higher, the more similar to swiss cheese
            unsigned octaves = 12; // The "granularity" of the map. Look up Fractal Brownian Motion
        } altitudeNoise;

        nlohmann::json serialize() const;
        static WorldGeneratorConfig deserialize(const nlohmann::json& j);
    };
}
