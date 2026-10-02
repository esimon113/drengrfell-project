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

	void Hero::setMovedThisTurn(bool moved) {
		movedThisTurn = moved;
	}

	bool Hero::hasMovedThisTurn() const {
		return movedThisTurn;
	}

	nlohmann::json Hero::serialize() const {
		nlohmann::json j;
		j["tileID"] = tileID;
		j["coords"] = {coords.x, coords.y};
		j["baseRange"] = baseRange;
		j["movedThisTurn"] = movedThisTurn;
		return j;
	}

	void Hero::deserialize(const nlohmann::json& j) {
		if (j.contains("tileID")) {
			tileID = j.at("tileID").get<size_t>();
		}
		if (j.contains("coords") && j["coords"].is_array() && j["coords"].size() >= 2) {
			coords.x = j["coords"][0].get<float>();
			coords.y = j["coords"][1].get<float>();
		}
		if (j.contains("baseRange")) {
			baseRange = j.at("baseRange").get<int>();
		}
		if (j.contains("movedThisTurn")) {
			movedThisTurn = j.at("movedThisTurn").get<bool>();
		}
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
