#include "hero.h"

namespace df {

    Hero::Hero() = default;

    Hero::Hero(int tileID, const glm::vec2& coords, const std::string& textureRef, int baseRange)
        : tileID(tileID),
        coords(coords),
        textureRef(textureRef),
        baseRange(baseRange),
        currentAnim(nullptr)
    { }


    void Hero::setTileID(int id) { this->tileID = id; }
    int Hero::getTileID() const { return this->tileID; }


    void Hero::setCoords(const glm::vec2& pos) { this->coords = pos; }
    const glm::vec2& Hero::getCoords() const { return this->coords; }


    void Hero::setTextureRef(const std::string& ref) { this->textureRef = ref; }
    const std::string& Hero::getTextureRef() const { return this->textureRef; }


    void Hero::setBaseRange(int range) { this->baseRange = range; }
    int Hero::getBaseRange() const { return this->baseRange; }


    void Hero::setIdleAnimation(const Animation& anim) { this->idleAnim = anim; }
    void Hero::setWalkAnimation(const Animation& anim) { this->walkAnim = anim; }


    void Hero::startIdleAnimation() { this->currentAnim = &this->idleAnim; }
    void Hero::startWalkAnimation() { this->currentAnim = &this->walkAnim; }

    
    void Hero::updateAnimation(float deltaTime) {
        if (this->currentAnim) {
            this->currentAnim->step(deltaTime);
        }
    }

}
