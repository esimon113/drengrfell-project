#pragma once
#include "animations.h"
#include "hero.h"
#include "types.h"

namespace df {

	struct AnimationComponent {
		Animation anim;
		Hero::AnimationType currentType = Hero::AnimationType::Idle;

		std::string currentFrame() const { return anim.getCurrentFrame(); }
	};

	enum class BuildingPreviewType {
		Settlement,
		Road
	};

	struct BuildingPreviewComponent {
		BuildingPreviewType type = BuildingPreviewType::Settlement;
	};

	struct HazardComponent {
		types::HazardType type;
		int turnsLeft;
	};
} // namespace df
