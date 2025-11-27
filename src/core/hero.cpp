#include <string>
#include <unordered_map>
#include <vector>
#include <glm/vec2.hpp>
#include "animations.h"
#include "hero.h"

namespace df {

    Hero::Hero() = default;

    Hero::Hero(int tileID, const glm::vec2& coords, const std::string& textureRef = "", int baseRange = 3)
        : tileID(tileID),
        coords(coords),
        textureRef(textureRef),
        baseRange(baseRange),
        currentAnim(nullptr)
    {
    }

    // Position
    void Hero::setTileID(int id) { tileID = id; }
    int Hero::getTileID() const { return tileID; }

    void Hero::setCoords(const glm::vec2& pos) { coords = pos; }
    const glm::vec2& Hero::getCoords() const { return coords; }

    // Texture reference (optional)
    void Hero::setTextureRef(const std::string& ref) { textureRef = ref; }
    const std::string& Hero::getTextureRef() const { return textureRef; }

    void Hero::setBaseRange(int range) { baseRange = range; }
    int Hero::getBaseRange() const { return baseRange; }

    // Animationen verwalten
    void Hero::setAnimation(const std::string& name, const std::vector<int>& frames, float frameDuration, bool loop = true) {
        Animation anim(frames, frameDuration, loop);
        animations[name] = anim;
    }

    void Hero::startAnimation(const std::string& name) {
        auto it = animations.find(name);
        if (it != animations.end()) {
            currentAnim = &it->second;
        }
    }

    void Hero::updateAnimation(float deltaTime) {
        if (currentAnim) {
            currentAnim->step(deltaTime);
        }
    }



}
