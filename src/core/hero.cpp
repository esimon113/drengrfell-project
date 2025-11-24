#pragma once
#include <string>
#include <glm/vec2.hpp>
#include "animations.h"
#include "tile.h"

namespace df {

    class Hero {
    public:
        Hero() = default;

        Hero(int tileID, const glm::vec2& coords, const std::string& textureRef, int baseRange)
            : tileID(tileID),
            coords(coords),
            textureRef(textureRef),
            baseRange(baseRange),
            currentAnim(nullptr)
        {
        }

        // position on map
        void setTileID(int id) { this->tileID = id; }
        int getTileID() const { return this->tileID; }

        void setCoords(const glm::vec2& pos) { this->coords = pos; }
        const glm::vec2& getCoords() const { return this->coords; }

        // texture reference
        void setTextureRef(const std::string& ref) { this->textureRef = ref; }
        const std::string& getTextureRef() const { return this->textureRef; }

        // basis range to move
        void setBaseRange(int range) { this->baseRange = range; }
        int getBaseRange() const { return this->baseRange; }

        // animation change
        void setIdleAnimation(const Animation& anim) { this->idleAnim = anim; }
        void setWalkAnimation(const Animation& anim) { this->walkAnim = anim; }

        void startIdleAnimation() { this->currentAnim = &this->idleAnim; }
        void startWalkAnimation() { this->currentAnim = &this->walkAnim; }

        void updateAnimation(float deltaTime) {
            if (this->currentAnim) {
                this->currentAnim->step(deltaTime);
            }
        }

    private:
        int tileID = -1;                 
        // current Tile-ID
        glm::vec2 coords{ 0.f, 0.f };    
        // Position on Tile
        std::string textureRef;          
        // Referenz for texture
        int baseRange = 3;               
        // maxi tile range

        // Animationen
        Animation idleAnim;
        Animation walkAnim;
        Animation* currentAnim;          
        // Pointer on current running Animation
    };

} 
