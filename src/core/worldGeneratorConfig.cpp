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
            }},
            {"useWhittakerBiomes", useWhittakerBiomes},
            {"temperatureNoise", {
                {"frequency", temperatureNoise.frequency},
                {"persistence", temperatureNoise.persistence},
                {"octaves", temperatureNoise.octaves},
            }},
            {"precipitationNoise", {
                {"frequency", precipitationNoise.frequency},
                {"persistence", precipitationNoise.persistence},
                {"octaves", precipitationNoise.octaves},
            }},
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
        overwrite(j, "useWhittakerBiomes", self.useWhittakerBiomes);

        const auto a = j.value("altitudeNoise", json::object());
        overwrite(a, "frequency", self.altitudeNoise.frequency);
        overwrite(a, "persistence", self.altitudeNoise.persistence);
        overwrite(a, "octaves", self.altitudeNoise.octaves);

        const auto t = j.value("temperatureNoise", json::object());
        overwrite(t, "frequency", self.temperatureNoise.frequency);
        overwrite(t, "persistence", self.temperatureNoise.persistence);
        overwrite(t, "octaves", self.temperatureNoise.octaves);

        const auto p = j.value("precipitationNoise", json::object());
        overwrite(p, "frequency", self.precipitationNoise.frequency);
        overwrite(p, "persistence", self.precipitationNoise.persistence);
        overwrite(p, "octaves", self.precipitationNoise.octaves);

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
