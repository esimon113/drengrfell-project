#pragma once

#include <string>
#include <glm/vec2.hpp>

#include "utils/animations.h"



namespace df {
    class Animation;

    class Hero {
    public:
        Hero();
        Hero(int tileID, const glm::vec2& coords, const std::string& textureRef, int baseRange);

        // position on map
        void setTileID(int id);
        int getTileID() const;

        void setCoords(const glm::vec2& pos);
        const glm::vec2& getCoords() const;

        // Texture reference
        void setTextureRef(const std::string& ref);
        const std::string& getTextureRef() const;

        // Basic range
        void setBaseRange(int range);
        int getBaseRange() const;

        // Animations (suspect of change)
        void setIdleAnimation(const Animation& anim);
        void setWalkAnimation(const Animation& anim);

        void startIdleAnimation();
        void startWalkAnimation();

        void updateAnimation(float deltaTime);

    private:
        int tileID = -1;
        // current Tile-ID
        glm::vec2 coords{ 0.f, 0.f };
        // Position on Tile
        std::string textureRef;
        // reference on texture
        int baseRange = 3;
        // maxi tile range

        // Animations
        Animation idleAnim;
        Animation walkAnim;
        Animation* currentAnim;
        // Pointer on current Animation
    };

}
