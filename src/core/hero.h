#pragma once
#include <string>
#include <glm/vec2.hpp>
#include <unordered_map>

#include "animations.h"

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

        void setAnimation(const std::string& name, const std::vector<int>& frames, float frameDuration, bool loop);
        void startAnimation(const std::string& name);
        void updateAnimation(float deltaTime);




        enum class AnimationType {
            Idle,
            Jump,
            Attack,
            Swim
        };

        struct HeroAnimations {
            inline static const std::vector<int> idle = { 0,1,2,1 };
            inline static const std::vector<int> swim = { 0, 1, 2, 3, 4, 5 };
            inline static const std::vector<int> jump = { 0, 5, 1, 2, 3, 5 };
            inline static const std::vector<int> attack = { 0,1 };
        };

    private:
        int tileID = -1;
        glm::vec2 coords{ 0.f, 0.f };
        std::string textureRef;
        int baseRange = 3;

        std::unordered_map<std::string, Animation> animations;
        Animation* currentAnim = nullptr;

    };

}
