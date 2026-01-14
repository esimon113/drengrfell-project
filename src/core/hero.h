#pragma once

#include <glm/vec2.hpp>
#include <string>
#include <unordered_map>

#include "utils/animations.h"



namespace df {
	class Animation;

	class Hero {
	  public:
		Hero();
		Hero(size_t tileID, const glm::vec2& coords, const std::string& textureRef, int baseRange);


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
		void setTileID(size_t id);
		size_t getTileID() const;

		enum class AnimationType {
			Idle,
			Jump,
			Attack,
			Swim,
			Run
		};

		struct HeroAnimations {
			inline static const std::vector<int> idle = {0, 1, 2, 1};
			inline static const std::vector<int> swim = {0, 1, 2, 3, 4, 5};
			inline static const std::vector<int> jump = {0, 5, 1, 2, 3, 5};
			inline static const std::vector<int> attack = {0, 1};
			inline static const std::vector<int> run = {0, 1, 2, 3, 4, 5};
		};

	  private:
		size_t tileID;
		glm::vec2 coords{0.f, 0.f};
		std::string textureRef;
		int baseRange = 3;

		std::unordered_map<std::string, Animation> animations;
		Animation* currentAnim = nullptr;
	};

} // namespace df
