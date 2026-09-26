#pragma once

#include "gamestate.h"

#include <vector>

namespace df {

	inline std::vector<size_t> visibleHeroOwners(const GameState& state, size_t viewerId) {
		std::vector<size_t> owners;
		const Player* viewer = state.getPlayer(viewerId);
		for (const Player& player : state.getPlayers()) {
			if (!player.getHero()) {
				continue;
			}
			if (player.getId() == viewerId) {
				owners.push_back(player.getId());
				continue;
			}
			if (viewer && viewer->isTileExplored(player.getHero()->getTileID())) {
				owners.push_back(player.getId());
			}
		}
		return owners;
	}

}
