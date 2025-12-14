#pragma once

namespace df {
    struct WorldGeneratorConfig {
        enum class GenerationMode {
            RANDOM,
            PERLIN
        };
        GenerationMode generationMode = GenerationMode::PERLIN;

        // General
        unsigned seed = 123;
        unsigned columns = 24;
        unsigned rows = 24;

        // Noise
        float persistence = 0.5f;
        unsigned octaves = 1;
    };
}
