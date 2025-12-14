#pragma once

namespace df {
    struct WorldGeneratorConfig {
        enum class GenerationMode {
            INSULAR, // Like in the classic board game
            PERLIN,  // TODO: Find better name
        };
        GenerationMode generationMode = GenerationMode::INSULAR;

        // General
        unsigned seed = 123;
        unsigned columns = 24;
        unsigned rows = 24;

        // Noise
        float persistence = 0.5f;
        unsigned octaves = 1;
    };
}
