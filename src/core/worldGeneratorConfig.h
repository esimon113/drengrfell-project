#pragma once

namespace df {
    struct WorldGeneratorConfig {
        enum class GenerationMode {
            INSULAR, // Like in the classic board game
            PERLIN,  // TODO: Find better name
        };
        GenerationMode generationMode = GenerationMode::PERLIN;

        // General
        unsigned seed = 123; // The same seed creates the same world
        unsigned columns = 24; // Size in x direction
        unsigned rows = 24; // Size in y direction

        // Noise
        struct AltitudeNoiseConfig {
            float frequency = 0.1f; // Same meaning as in sine waves
            float persistence = 0.50f; // The higher, the more similar to swiss cheese
            unsigned octaves = 12; // The "granularity" of the map. Look up Fractal Brownian Motion
        } altitudeNoise;

    };
}
