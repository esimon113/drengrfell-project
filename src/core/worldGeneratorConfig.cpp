#include "worldGeneratorConfig.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using json = nlohmann::json;


namespace df {
    json WorldGeneratorConfig::serialize() const {
        return {
            {"version", version},
            {"columns", columns},
            {"rows", rows},
            {"generationMode", generationMode},
            {"seed", seed},
            {"altitudeNoise", {
                {"frequency", altitudeNoise.frequency},
                {"persistence", altitudeNoise.persistence},
                {"octaves", altitudeNoise.octaves},
            }}
        };
    }

    template <class T>
    void overwrite(const nlohmann::json& j, const char* key, T& property) {
        property = j.value(key, property);
    }

    WorldGeneratorConfig WorldGeneratorConfig::deserialize(const json& j) {
        WorldGeneratorConfig self;

        overwrite(j, "version", self.version);
        overwrite(j, "columns", self.columns);
        overwrite(j, "rows", self.rows);
        overwrite(j, "generationMode", self.generationMode);
        overwrite(j, "seed", self.seed);

        const auto a = j.value("altitudeNoise", json::object());
        overwrite(a, "frequency", self.altitudeNoise.frequency);
        overwrite(a, "persistence", self.altitudeNoise.persistence);
        overwrite(a, "octaves", self.altitudeNoise.octaves);

        return self;
    }

    Result<WorldGeneratorConfig, ResultError> WorldGeneratorConfig::deserialize(const assets::JsonFile asset) {
        auto path = assets::getAssetPath(asset);
        std::ifstream file(path);
        if (!file) {
            return Err(ResultError(ResultError::Kind::IOError, "WorldGeneratorConfig::deserialize(): Could not open file: " + path));
        }

        try {
            json j;
            file >> j;
            return Ok(deserialize(j));
        } catch (const json::parse_error& e) {
            return Err(ResultError(ResultError::Kind::JsonParseError, "WorldGeneratorConfig::deserialize(): Could not parse file: " + path + ". Reason: " + std::string(e.what())));
        }
    }
}
