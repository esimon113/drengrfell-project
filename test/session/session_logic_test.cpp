#include "constructionCosts.h"
#include "multiplayer/sessionManager.h"
#include "player.h"
#include "utils/commandLineOptions.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <variant>

namespace {

int resourceAmount(const nlohmann::json& state, size_t playerId, df::types::TileType type) {
	if (!state.contains("players")) {
		return 0;
	}
	const std::string key = std::to_string(static_cast<int>(type));
	for (const auto& playerJson : state["players"]) {
		if (playerJson.value("playerId", static_cast<size_t>(0)) != playerId || !playerJson.contains("resources")) {
			continue;
		}
		return playerJson["resources"].value(key, 0);
	}
	return 0;
}

int questProgress(const nlohmann::json& state, int questId) {
	if (!state.contains("quests") || !state["quests"].is_array()) {
		return -999;
	}
	for (const auto& quest : state["quests"]) {
		if (quest.value("id", -1) == questId) {
			return quest.value("progress", -999);
		}
	}
	return -999;
}

bool resourcesUnchanged(const nlohmann::json& before, const nlohmann::json& after, size_t playerId) {
	using df::types::TileType;
	for (TileType type : {TileType::FOREST, TileType::GRASS, TileType::MOUNTAIN, TileType::FIELD, TileType::CLAY}) {
		if (resourceAmount(before, playerId, type) != resourceAmount(after, playerId, type)) {
			return false;
		}
	}
	return true;
}

}

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

	if (session.endTurn(10)) {
		std::cerr << "player 0 ended player 1's turn\n";
		return EXIT_FAILURE;
	}

	const auto state = session.getSerializedGameState();
	if (!state.contains("currentPlayerId") || state["currentPlayerId"].get<size_t>() != 1) {
		std::cerr << "currentPlayerId not advanced\n";
		return EXIT_FAILURE;
	}

	if (!session.endTurn(11)) {
		std::cerr << "player 1 could not end their own turn\n";
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
		df::bifrost::LobbyConfig soloConfig;
		soloConfig.solo = true;
		if (!solo.updateConfig(20, soloConfig)) {
			std::cerr << "solo config failed\n";
			return EXIT_FAILURE;
		}
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

	{
		df::bifrost::SessionManager costs;
		if (!costs.addClient(30, "Builder")) {
			std::cerr << "cost session failed to start\n";
			return EXIT_FAILURE;
		}
		costs.setPlayerReady(30, true);
		df::bifrost::LobbyConfig soloConfig;
		soloConfig.solo = true;
		if (!costs.updateConfig(30, soloConfig)) {
			std::cerr << "solo config failed\n";
			return EXIT_FAILURE;
		}
		if (!costs.startGame(30)) {
			std::cerr << "cost session failed to start\n";
			return EXIT_FAILURE;
		}

		const auto before = costs.getSerializedGameState();
		bool built = false;
		for (size_t vertexId = 576; vertexId < 2500; ++vertexId) {
			if (costs.buildSettlement(30, vertexId).first) {
				built = true;
				break;
			}
		}
		if (!built) {
			std::cerr << "could not place a settlement to check costs\n";
			return EXIT_FAILURE;
		}

		const auto placed = costs.getSerializedGameState();
		if (!placed.contains("settlements") || !placed["settlements"].is_array() || placed["settlements"].empty()) {
			std::cerr << "settlement was not recorded\n";
			return EXIT_FAILURE;
		}

		const auto& expectedSettlement = df::settlementPlacementCost();
		using df::types::TileType;
		for (TileType type : {TileType::FOREST, TileType::GRASS, TileType::MOUNTAIN, TileType::FIELD, TileType::CLAY}) {
			const size_t index = static_cast<size_t>(type);
			const int delta = resourceAmount(before, 0, type) - resourceAmount(placed, 0, type);
			if (index >= expectedSettlement.size() || delta != expectedSettlement[index]) {
				std::cerr << "settlement cost was not charged\n";
				return EXIT_FAILURE;
			}
		}

		const size_t settlementId = placed["settlements"][0].value("id", static_cast<size_t>(0));
		const auto upgrade = costs.upgradeSettlement(30, settlementId, df::types::SettlementType::STONE);
		const auto afterUpgrade = costs.getSerializedGameState();
		if (upgrade.first || !resourcesUnchanged(placed, afterUpgrade, 0)) {
			std::cerr << "stone upgrade charged the wrong cost\n";
			return EXIT_FAILURE;
		}

		bool productivityBuilt = false;
		for (size_t tileId = 0; tileId < 576 && !productivityBuilt; ++tileId) {
			for (TileType type : {TileType::FOREST, TileType::MOUNTAIN, TileType::GRASS, TileType::FIELD, TileType::CLAY}) {
				if (costs.buildProductivityBuilding(30, tileId, type).first) {
					productivityBuilt = true;
					break;
				}
			}
		}
		const auto afterProductivity = costs.getSerializedGameState();
		if (productivityBuilt || !resourcesUnchanged(placed, afterProductivity, 0)) {
			std::cerr << "productivity building charged the wrong cost\n";
			return EXIT_FAILURE;
		}
	}

	{
		df::bifrost::SessionManager quests;
		if (!quests.addClient(40, "Quester")) {
			std::cerr << "quest session failed to start\n";
			return EXIT_FAILURE;
		}
		quests.setPlayerReady(40, true);
		df::bifrost::LobbyConfig soloConfig;
		soloConfig.solo = true;
		if (!quests.updateConfig(40, soloConfig)) {
			std::cerr << "solo config failed\n";
			return EXIT_FAILURE;
		}
		if (!quests.startGame(40)) {
			std::cerr << "quest session failed to start\n";
			return EXIT_FAILURE;
		}
		if (!quests.claimQuest(40, 0).first) {
			std::cerr << "could not claim the tutorial quest\n";
			return EXIT_FAILURE;
		}

		const auto afterClaim = quests.getSerializedGameState();
		const int settlementsBefore = questProgress(afterClaim, 1);
		if (settlementsBefore < 0) {
			std::cerr << "settlement quest was not in the snapshot\n";
			return EXIT_FAILURE;
		}

		bool built = false;
		for (size_t vertexId = 576; vertexId < 2500; ++vertexId) {
			if (quests.buildSettlement(40, vertexId).first) {
				built = true;
				break;
			}
		}
		if (!built) {
			std::cerr << "could not build a settlement for quest progress\n";
			return EXIT_FAILURE;
		}

		const auto afterBuild = quests.getSerializedGameState();
		const int settlementsAfter = questProgress(afterBuild, 1);
		if (settlementsAfter != settlementsBefore + 1) {
			std::cerr << "settlement quest progress did not advance\n";
			return EXIT_FAILURE;
		}

		df::QuestsSystem shown;
		shown.init(nullptr);
		shown.applyAuthoritative(afterClaim["quests"]);
		const df::Quest* shownQuest = shown.getQuestById(1);
		if (!shownQuest || shownQuest->progress != settlementsBefore) {
			std::cerr << "quest window did not take the first snapshot\n";
			return EXIT_FAILURE;
		}
		shown.applyAuthoritative(afterBuild["quests"]);
		shownQuest = shown.getQuestById(1);
		if (!shownQuest || shownQuest->progress != settlementsAfter) {
			std::cerr << "quest window did not take the updated snapshot\n";
			return EXIT_FAILURE;
		}
	}

	{
		df::bifrost::SessionManager alone;
		if (!alone.addClient(50, "Only")) {
			std::cerr << "alone addClient failed\n";
			return EXIT_FAILURE;
		}
		alone.setPlayerReady(50, true);
		if (alone.startGame(50)) {
			std::cerr << "one player started a multiplayer match\n";
			return EXIT_FAILURE;
		}

		df::bifrost::LobbyConfig soloConfig;
		soloConfig.solo = true;
		if (!alone.updateConfig(50, soloConfig)) {
			std::cerr << "host could not set solo\n";
			return EXIT_FAILURE;
		}
		if (!alone.startGame(50)) {
			std::cerr << "solo start failed\n";
			return EXIT_FAILURE;
		}
	}

	{
		char soloProgram[] = "drengrfell";
		char soloFlag[] = "--solo";
		char* withSolo[] = {soloProgram, soloFlag};
		const df::CommandLineOptions parsed = df::CommandLineOptions::parse(2, withSolo);
		if (!parsed.isSolo()) {
			std::cerr << "--solo was not recognized\n";
			return EXIT_FAILURE;
		}
		char plainProgram[] = "drengrfell";
		char* without[] = {plainProgram};
		const df::CommandLineOptions plain = df::CommandLineOptions::parse(1, without);
		if (plain.isSolo()) {
			std::cerr << "solo defaulted on\n";
			return EXIT_FAILURE;
		}
	}

	std::cout << "session_logic_test: initializeGame, endTurn, serializeFor OK\n";
	return EXIT_SUCCESS;
}
