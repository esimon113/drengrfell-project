#include "multiplayer/sessionManager.h"
#include "player.h"

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

	{
		df::bifrost::SessionManager solo;
		if (!solo.addClient(20, "Solo")) {
			std::cerr << "solo addClient failed\n";
			return EXIT_FAILURE;
		}
		solo.setPlayerReady(20, true);
		if (!solo.startGame(20)) {
			std::cerr << "one player could not start\n";
			return EXIT_FAILURE;
		}
		const auto soloState = solo.getSerializedGameState();
		if (!soloState.contains("currentTutorialStep") || !soloState.contains("weather") || !soloState.contains("weatherIntensity")) {
			std::cerr << "snapshot missing tutorial or weather\n";
			return EXIT_FAILURE;
		}
		if (!solo.endTurn(20)) {
			std::cerr << "solo endTurn failed\n";
			return EXIT_FAILURE;
		}

		const auto beforeClaim = solo.getSerializedGameState();
		int forestBefore = 0;
		if (beforeClaim.contains("players")) {
			for (const auto& playerJson : beforeClaim["players"]) {
				if (playerJson.value("playerId", static_cast<size_t>(0)) == 0 && playerJson.contains("resources")) {
					forestBefore = playerJson["resources"].value("2", 0);
				}
			}
		}
		const auto claimed = solo.claimQuest(20, 0);
		if (!claimed.first) {
			std::cerr << "claim quest failed\n";
			return EXIT_FAILURE;
		}
		const auto afterClaim = solo.getSerializedGameState();
		int forestAfter = 0;
		if (afterClaim.contains("players")) {
			for (const auto& playerJson : afterClaim["players"]) {
				if (playerJson.value("playerId", static_cast<size_t>(0)) == 0 && playerJson.contains("resources")) {
					forestAfter = playerJson["resources"].value("2", 0);
				}
			}
		}
		if (forestAfter != forestBefore + 5) {
			std::cerr << "quest reward was not granted\n";
			return EXIT_FAILURE;
		}
		if (solo.claimQuest(20, 0).first) {
			std::cerr << "quest was claimed twice\n";
			return EXIT_FAILURE;
		}
	}

	{
		df::Player player(4);
		df::Player::ActiveHazard hazard;
		hazard.type = df::types::HazardType::BEAR;
		hazard.turnsLeft = 2;
		player.setActiveHazard(hazard);
		const auto hazardJson = player.serialize();
		if (!hazardJson.contains("activeHazard") || hazardJson["activeHazard"].value("turnsLeft", 0) != 2) {
			std::cerr << "active hazard was not serialized\n";
			return EXIT_FAILURE;
		}
		df::Player restored;
		restored.deserialize(hazardJson);
		if (!restored.hasActiveHazard() || restored.getActiveHazard()->type != df::types::HazardType::BEAR ||
			restored.getActiveHazard()->turnsLeft != 2) {
			std::cerr << "active hazard did not roundtrip\n";
			return EXIT_FAILURE;
		}
		df::Player clearPlayer(5);
		if (clearPlayer.serialize().contains("activeHazard")) {
			std::cerr << "missing hazard should be omitted\n";
			return EXIT_FAILURE;
		}
	}

	{
		df::bifrost::Message pay;
		pay.type = df::bifrost::MessageType::PAY_HAZARD;
		pay.seq = 3;
		pay.payload = df::bifrost::PayHazardPayload{};
		const auto decodedPay = df::bifrost::Message::deserialize(pay.serialize());
		if (decodedPay.type != df::bifrost::MessageType::PAY_HAZARD) {
			std::cerr << "PayHazard roundtrip failed\n";
			return EXIT_FAILURE;
		}

		df::bifrost::Message tutorial;
		tutorial.type = df::bifrost::MessageType::TUTORIAL_EVENT;
		tutorial.seq = 4;
		tutorial.payload = df::bifrost::TutorialEventPayload{static_cast<int>(df::TutorialStepId::MOVE_CAMERA)};
		const auto decodedTutorial = df::bifrost::Message::deserialize(tutorial.serialize());
		if (decodedTutorial.type != df::bifrost::MessageType::TUTORIAL_EVENT) {
			std::cerr << "TutorialEvent roundtrip type failed\n";
			return EXIT_FAILURE;
		}
		const auto& tutorialPayload = std::get<df::bifrost::TutorialEventPayload>(decodedTutorial.payload);
		if (tutorialPayload.stepId != static_cast<int>(df::TutorialStepId::MOVE_CAMERA)) {
			std::cerr << "TutorialEvent roundtrip payload failed\n";
			return EXIT_FAILURE;
		}
	}

	std::cout << "session_logic_test: initializeGame, endTurn, serializeFor OK\n";
	return EXIT_SUCCESS;
}
