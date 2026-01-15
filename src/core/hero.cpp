#include "hero.h"
#include "animations.h"
#include <glm/vec2.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace df {

	Hero::Hero() = default;

	Hero::Hero(size_t tileID, const glm::vec2& coords, const std::string& textureRef = "", int baseRange = 3)
		: tileID(tileID),
		  coords(coords),
		  textureRef(textureRef),
		  baseRange(baseRange),
		  currentAnim(nullptr) {
	}

	void Hero::setCoords(const glm::vec2& pos) { coords = pos; }
	const glm::vec2& Hero::getCoords() const { return coords; }

	// Texture reference (optional)
	void Hero::setTextureRef(const std::string& ref) { textureRef = ref; }
	const std::string& Hero::getTextureRef() const { return textureRef; }

	void Hero::setBaseRange(int range) { baseRange = range; }
	int Hero::getBaseRange() const { return baseRange; }

	void Hero::setTileID(size_t id) {
		tileID = id;
	}

	size_t Hero::getTileID() const {
		return tileID;
	}

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
} // namespace df
