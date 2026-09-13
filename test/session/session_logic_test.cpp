#include "multiplayer/sessionManager.h"

#include <cstdlib>
#include <iostream>
#include <variant>

int main() {
	df::bifrost::SessionManager session;
	if (!session.addClient(10, "VikingA") || !session.addClient(11, "VikingB")) {
		std::cerr << "addClient failed\n";
		return EXIT_FAILURE;
	}

	session.setPlayerReady(10, true);
	session.setPlayerReady(11, true);
	if (!session.startGame(10)) {
		std::cerr << "startGame failed\n";
		return EXIT_FAILURE;
	}

	if (session.getState() != df::bifrost::SessionState::PLAYING) {
		std::cerr << "session not PLAYING\n";
		return EXIT_FAILURE;
	}

	if (!session.endTurn(10)) {
		std::cerr << "endTurn failed\n";
		return EXIT_FAILURE;
	}

	const auto state = session.getSerializedGameState();
	if (!state.contains("currentPlayerId") || state["currentPlayerId"].get<size_t>() != 1) {
		std::cerr << "currentPlayerId not advanced\n";
		return EXIT_FAILURE;
	}

	if (!state.contains("world") || !state["world"].contains("seed") || state["world"]["seed"].get<unsigned>() == 0) {
		std::cerr << "resolved world seed missing\n";
		return EXIT_FAILURE;
	}

	const auto build = session.buildSettlement(11, 0);
	if (!build.first && !build.second) {
		std::cerr << "buildSettlement returned malformed error\n";
		return EXIT_FAILURE;
	}

	{
		df::bifrost::Message upgrade;
		upgrade.type = df::bifrost::MessageType::UPGRADE_SETTLEMENT;
		upgrade.seq = 7;
		upgrade.payload = df::bifrost::UpgradeSettlementPayload{3, df::types::SettlementType::STONE};
		const auto decoded = df::bifrost::Message::deserialize(upgrade.serialize());
		if (decoded.type != df::bifrost::MessageType::UPGRADE_SETTLEMENT) {
			std::cerr << "UpgradeSettlement roundtrip type failed\n";
			return EXIT_FAILURE;
		}
		const auto& payload = std::get<df::bifrost::UpgradeSettlementPayload>(decoded.payload);
		if (payload.settlementId != 3 || payload.targetType != df::types::SettlementType::STONE) {
			std::cerr << "UpgradeSettlement roundtrip payload failed\n";
			return EXIT_FAILURE;
		}
	}

	const auto filtered = session.getSerializedGameStateForSocket(10);
	if (filtered.contains("players") && filtered["players"].is_array()) {
		for (const auto& playerJson : filtered["players"]) {
			if (!playerJson.is_object()) {
				continue;
			}
			if (playerJson.value("playerId", static_cast<size_t>(0)) == 1 && playerJson.contains("resources")) {
				std::cerr << "other player resources leaked to viewer 0\n";
				return EXIT_FAILURE;
			}
		}
	}

	std::cout << "session_logic_test: initializeGame, endTurn, serializeFor OK\n";
	return EXIT_SUCCESS;
}
