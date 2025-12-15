#include "worldGeneratorConfig.h"
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
}
