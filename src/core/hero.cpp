#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/vec2.hpp>
#include "animations.h"
#include "tile.h"

namespace df {

    class Hero {
    public:
        Hero() = default;

        Hero(int tileID, const glm::vec2& coords, const std::string& textureRef = "", int baseRange = 3)
            : tileID(tileID),
            coords(coords),
            textureRef(textureRef),
            baseRange(baseRange),
            currentAnim(nullptr)
        {
        }

        // Position
        void setTileID(int id) { tileID = id; }
        int getTileID() const { return tileID; }

        void setCoords(const glm::vec2& pos) { coords = pos; }
        const glm::vec2& getCoords() const { return coords; }

        // Texture reference (optional)
        void setTextureRef(const std::string& ref) { textureRef = ref; }
        const std::string& getTextureRef() const { return textureRef; }

        void setBaseRange(int range) { baseRange = range; }
        int getBaseRange() const { return baseRange; }

        // Animationen verwalten
        void setAnimation(const std::string& name, const std::vector<std::string>& frames, float frameDuration, bool loop = true) {
            Animation anim(frames, frameDuration, loop);
            animations[name] = anim;
        }

        void startAnimation(const std::string& name) {
            auto it = animations.find(name);
            if (it != animations.end()) {
                currentAnim = &it->second;
            }
        }

        void updateAnimation(float deltaTime) {
            if (currentAnim) {
                currentAnim->step(deltaTime);
            }
        }

        const std::string& getCurrentFrame() const {
            if (currentAnim) return currentAnim->getCurrentFrame();
            static const std::string empty = "";
            return empty;
        }

    private:
        int tileID = -1;
        glm::vec2 coords{ 0.f, 0.f };
        std::string textureRef;
        int baseRange = 3;

        std::unordered_map<std::string, Animation> animations;
        Animation* currentAnim = nullptr;
    };

}
