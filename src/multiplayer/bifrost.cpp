/**
 * @file bifrost.cpp
 * @brief Implementation of the Bifrost Protocol v1.0
 */

#include "bifrost.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

#include <fmt/core.h>


namespace df::bifrost {

	// ═══════════════════════════════════════════════════════════════════════════════
	// ERROR CODE CONVERSION
	// ═══════════════════════════════════════════════════════════════════════════════

	std::string errorCodeToString(ErrorCode code) {
		switch (code) {
		case ErrorCode::NONE:
			return "NONE";
		case ErrorCode::INVALID_MESSAGE:
			return "INVALID_MESSAGE";
		case ErrorCode::NOT_YOUR_TURN:
			return "NOT_YOUR_TURN";
		case ErrorCode::INVALID_ACTION:
			return "INVALID_ACTION";
		case ErrorCode::INSUFFICIENT_RESOURCES:
			return "INSUFFICIENT_RESOURCES";
		case ErrorCode::INVALID_LOCATION:
			return "INVALID_LOCATION";
		case ErrorCode::LOBBY_FULL:
			return "LOBBY_FULL";
		case ErrorCode::NAME_TAKEN:
			return "NAME_TAKEN";
		case ErrorCode::NOT_HOST:
			return "NOT_HOST";
		case ErrorCode::GAME_ALREADY_STARTED:
			return "GAME_ALREADY_STARTED";
		case ErrorCode::NOT_ALL_READY:
			return "NOT_ALL_READY";
		case ErrorCode::PLAYER_NOT_FOUND:
			return "PLAYER_NOT_FOUND";
		case ErrorCode::CONNECTION_LOST:
			return "CONNECTION_LOST";
		case ErrorCode::TIMEOUT:
			return "TIMEOUT";
		case ErrorCode::INTERNAL_ERROR:
			return "INTERNAL_ERROR";
		}
		return "UNKNOWN";
	}

	ErrorCode stringToErrorCode(const std::string& str) {
		if (str == "NONE")
			return ErrorCode::NONE;
		if (str == "INVALID_MESSAGE")
			return ErrorCode::INVALID_MESSAGE;
		if (str == "NOT_YOUR_TURN")
			return ErrorCode::NOT_YOUR_TURN;
		if (str == "INVALID_ACTION")
			return ErrorCode::INVALID_ACTION;
		if (str == "INSUFFICIENT_RESOURCES")
			return ErrorCode::INSUFFICIENT_RESOURCES;
		if (str == "INVALID_LOCATION")
			return ErrorCode::INVALID_LOCATION;
		if (str == "LOBBY_FULL")
			return ErrorCode::LOBBY_FULL;
		if (str == "NAME_TAKEN")
			return ErrorCode::NAME_TAKEN;
		if (str == "NOT_HOST")
			return ErrorCode::NOT_HOST;
		if (str == "GAME_ALREADY_STARTED")
			return ErrorCode::GAME_ALREADY_STARTED;
		if (str == "NOT_ALL_READY")
			return ErrorCode::NOT_ALL_READY;
		if (str == "PLAYER_NOT_FOUND")
			return ErrorCode::PLAYER_NOT_FOUND;
		if (str == "CONNECTION_LOST")
			return ErrorCode::CONNECTION_LOST;
		if (str == "TIMEOUT")
			return ErrorCode::TIMEOUT;
		if (str == "INTERNAL_ERROR")
			return ErrorCode::INTERNAL_ERROR;
		return ErrorCode::INTERNAL_ERROR;
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// MESSAGE TYPE CONVERSION
	// ═══════════════════════════════════════════════════════════════════════════════

	std::string messageTypeToString(MessageType type) {
		switch (type) {
		case MessageType::JOIN_REQUEST:
			return "JoinRequest";
		case MessageType::LEAVE_REQUEST:
			return "LeaveRequest";
		case MessageType::READY_TOGGLE:
			return "ReadyToggle";
		case MessageType::UPDATE_CONFIG:
			return "UpdateConfig";
		case MessageType::START_GAME:
			return "StartGame";
		case MessageType::KICK_PLAYER:
			return "KickPlayer";
		case MessageType::END_TURN:
			return "EndTurn";
		case MessageType::BUILD_SETTLEMENT:
			return "BuildSettlement";
		case MessageType::BUILD_ROAD:
			return "BuildRoad";
		case MessageType::MOVE_HERO:
			return "MoveHero";
		case MessageType::PING:
			return "Ping";
		case MessageType::RECONNECT:
			return "Reconnect";
		case MessageType::JOIN_RESPONSE:
			return "JoinResponse";
		case MessageType::LOBBY_STATE:
			return "LobbyState";
		case MessageType::CONFIG_UPDATE:
			return "ConfigUpdate";
		case MessageType::KICKED:
			return "Kicked";
		case MessageType::GAME_STARTED:
			return "GameStarted";
		case MessageType::GAME_STATE:
			return "GameState";
		case MessageType::ACTION_RESULT:
			return "ActionResult";
		case MessageType::GAME_PAUSED:
			return "GamePaused";
		case MessageType::GAME_RESUMED:
			return "GameResumed";
		case MessageType::GAME_OVER:
			return "GameOver";
		case MessageType::PONG:
			return "Pong";
		case MessageType::RECONNECT_RESPONSE:
			return "ReconnectResponse";
		case MessageType::SERVER_ERROR:
			return "ServerError";
		case MessageType::UNKNOWN:
			return "Unknown";
		}
		return "Unknown";
	}

	MessageType stringToMessageType(const std::string& str) {
		if (str == "JoinRequest")
			return MessageType::JOIN_REQUEST;
		if (str == "LeaveRequest")
			return MessageType::LEAVE_REQUEST;
		if (str == "ReadyToggle")
			return MessageType::READY_TOGGLE;
		if (str == "UpdateConfig")
			return MessageType::UPDATE_CONFIG;
		if (str == "StartGame")
			return MessageType::START_GAME;
		if (str == "KickPlayer")
			return MessageType::KICK_PLAYER;
		if (str == "EndTurn")
			return MessageType::END_TURN;
		if (str == "BuildSettlement")
			return MessageType::BUILD_SETTLEMENT;
		if (str == "BuildRoad")
			return MessageType::BUILD_ROAD;
		if (str == "MoveHero")
			return MessageType::MOVE_HERO;
		if (str == "Ping")
			return MessageType::PING;
		if (str == "Reconnect")
			return MessageType::RECONNECT;
		if (str == "JoinResponse")
			return MessageType::JOIN_RESPONSE;
		if (str == "LobbyState")
			return MessageType::LOBBY_STATE;
		if (str == "ConfigUpdate")
			return MessageType::CONFIG_UPDATE;
		if (str == "Kicked")
			return MessageType::KICKED;
		if (str == "GameStarted")
			return MessageType::GAME_STARTED;
		if (str == "GameState")
			return MessageType::GAME_STATE;
		if (str == "ActionResult")
			return MessageType::ACTION_RESULT;
		if (str == "GamePaused")
			return MessageType::GAME_PAUSED;
		if (str == "GameResumed")
			return MessageType::GAME_RESUMED;
		if (str == "GameOver")
			return MessageType::GAME_OVER;
		if (str == "Pong")
			return MessageType::PONG;
		if (str == "ReconnectResponse")
			return MessageType::RECONNECT_RESPONSE;
		if (str == "ServerError")
			return MessageType::SERVER_ERROR;
		return MessageType::UNKNOWN;
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// DETAIL NAMESPACE - HELPER FUNCTIONS
	// ═══════════════════════════════════════════════════════════════════════════════

	namespace detail {

		std::string tileTypeToString(types::TileType type) {
			switch (type) {
			case types::TileType::EMPTY:
				return "EMPTY";
			case types::TileType::WATER:
				return "WATER";
			case types::TileType::FOREST:
				return "FOREST";
			case types::TileType::GRASS:
				return "GRASS";
			case types::TileType::MOUNTAIN:
				return "MOUNTAIN";
			case types::TileType::FIELD:
				return "FIELD";
			case types::TileType::CLAY:
				return "CLAY";
			case types::TileType::ICE:
				return "ICE";
			case types::TileType::COUNT:
				return "COUNT";
			}
			return "UNKNOWN";
		}

		types::TileType stringToTileType(const std::string& str) {
			if (str == "EMPTY")
				return types::TileType::EMPTY;
			if (str == "WATER")
				return types::TileType::WATER;
			if (str == "FOREST")
				return types::TileType::FOREST;
			if (str == "GRASS")
				return types::TileType::GRASS;
			if (str == "MOUNTAIN")
				return types::TileType::MOUNTAIN;
			if (str == "FIELD")
				return types::TileType::FIELD;
			if (str == "CLAY")
				return types::TileType::CLAY;
			if (str == "ICE")
				return types::TileType::ICE;
			return types::TileType::EMPTY;
		}

		std::string roadLevelToString(bifrost::RoadLevel level) {
			switch (level) {
			case bifrost::RoadLevel::Path:
				return "Path";
			case bifrost::RoadLevel::DirtRoad:
				return "DirtRoad";
			case bifrost::RoadLevel::StoneRoad:
				return "StoneRoad";
			case bifrost::RoadLevel::HighQualityRoad:
				return "HighQualityRoad";
			}
			return "Path";
		}

		bifrost::RoadLevel stringToRoadLevel(const std::string& str) {
			if (str == "Path")
				return bifrost::RoadLevel::Path;
			if (str == "DirtRoad")
				return bifrost::RoadLevel::DirtRoad;
			if (str == "StoneRoad")
				return bifrost::RoadLevel::StoneRoad;
			if (str == "HighQualityRoad")
				return bifrost::RoadLevel::HighQualityRoad;
			return bifrost::RoadLevel::Path;
		}


		nlohmann::json serializePayload(MessageType type, const MessagePayload& payload) {
			nlohmann::json j;

			switch (type) {
			case MessageType::JOIN_REQUEST: {
				const auto& p = std::get<JoinRequestPayload>(payload);
				j["playerName"] = p.playerName;
				break;
			}
			case MessageType::LEAVE_REQUEST:
				// Empty payload
				break;
			case MessageType::READY_TOGGLE: {
				const auto& p = std::get<ReadyTogglePayload>(payload);
				j["ready"] = p.ready;
				break;
			}
			case MessageType::UPDATE_CONFIG: {
				const auto& p = std::get<UpdateConfigPayload>(payload);
				j["config"] = p.config.serialize();
				break;
			}
			case MessageType::START_GAME:
				// Empty payload
				break;
			case MessageType::KICK_PLAYER: {
				const auto& p = std::get<KickPlayerPayload>(payload);
				j["playerName"] = p.playerName;
				break;
			}
			case MessageType::END_TURN:
				// Empty payload
				break;
			case MessageType::BUILD_SETTLEMENT: {
				const auto& p = std::get<BuildSettlementPayload>(payload);
				j["vertexId"] = p.vertexId;
				break;
			}
			case MessageType::BUILD_ROAD: {
				const auto& p = std::get<BuildRoadPayload>(payload);
				j["edgeId"] = p.edgeId;
				j["level"] = roadLevelToString(p.level);
				break;
			}
			case MessageType::MOVE_HERO: {
				const auto& p = std::get<MoveHeroPayload>(payload);
				j["targetTileId"] = p.targetTileId;
				break;
			}
			case MessageType::PING: {
				const auto& p = std::get<PingPayload>(payload);
				j["timestamp"] = p.timestamp;
				break;
			}
			case MessageType::RECONNECT: {
				const auto& p = std::get<ReconnectPayload>(payload);
				j["playerName"] = p.playerName;
				break;
			}
			case MessageType::JOIN_RESPONSE: {
				const auto& p = std::get<JoinResponsePayload>(payload);
				j["success"] = p.success;
				j["playerId"] = p.playerId;
				if (p.error) {
					j["error"] = p.error->serialize();
				} else {
					j["error"] = nullptr;
				}
				break;
			}
			case MessageType::LOBBY_STATE: {
				const auto& p = std::get<LobbyStatePayload>(payload);
				j = p.lobby.serialize();
				break;
			}
			case MessageType::CONFIG_UPDATE: {
				const auto& p = std::get<ConfigUpdatePayload>(payload);
				j["config"] = p.config.serialize();
				break;
			}
			case MessageType::KICKED: {
				const auto& p = std::get<KickedPayload>(payload);
				j["reason"] = p.reason;
				break;
			}
			case MessageType::GAME_STARTED: {
				const auto& p = std::get<GameStartedPayload>(payload);
				j["initialState"] = p.initialState;
				break;
			}
			case MessageType::GAME_STATE: {
				const auto& p = std::get<GameStatePayload>(payload);
				j["state"] = p.state;
				break;
			}
			case MessageType::ACTION_RESULT: {
				const auto& p = std::get<ActionResultPayload>(payload);
				j["seq"] = p.seq;
				j["success"] = p.success;
				if (p.error) {
					j["error"] = p.error->serialize();
				} else {
					j["error"] = nullptr;
				}
				break;
			}
			case MessageType::GAME_PAUSED: {
				const auto& p = std::get<GamePausedPayload>(payload);
				j["disconnectedPlayer"] = p.disconnectedPlayer;
				j["timeoutSeconds"] = p.timeoutSeconds;
				break;
			}
			case MessageType::GAME_RESUMED: {
				const auto& p = std::get<GameResumedPayload>(payload);
				j["reconnectedPlayer"] = p.reconnectedPlayer;
				break;
			}
			case MessageType::GAME_OVER: {
				const auto& p = std::get<GameOverPayload>(payload);
				j["winnerId"] = p.winnerId;
				nlohmann::json scoresJson = nlohmann::json::array();
				for (const auto& [playerId, score] : p.scores) {
					scoresJson.push_back({{"playerId", playerId}, {"score", score}});
				}
				j["scores"] = scoresJson;
				break;
			}
			case MessageType::PONG: {
				const auto& p = std::get<PongPayload>(payload);
				j["timestamp"] = p.timestamp;
				j["serverTime"] = p.serverTime;
				break;
			}
			case MessageType::RECONNECT_RESPONSE: {
				const auto& p = std::get<ReconnectResponsePayload>(payload);
				j["success"] = p.success;
				j["playerId"] = p.playerId;
				if (p.state) {
					j["state"] = *p.state;
				}
				if (p.error) {
					j["error"] = p.error->serialize();
				} else {
					j["error"] = nullptr;
				}
				break;
			}
			case MessageType::SERVER_ERROR: {
				const auto& p = std::get<ServerErrorPayload>(payload);
				j["code"] = errorCodeToString(p.code);
				j["message"] = p.message;
				break;
			}
			case MessageType::UNKNOWN:
				break;
			}

			return j;
		}


		MessagePayload deserializePayload(MessageType type, const nlohmann::json& j) {
			switch (type) {
			case MessageType::JOIN_REQUEST: {
				JoinRequestPayload p;
				p.playerName = j.value("playerName", "");
				return p;
			}
			case MessageType::LEAVE_REQUEST:
				return LeaveRequestPayload{};
			case MessageType::READY_TOGGLE: {
				ReadyTogglePayload p;
				p.ready = j.value("ready", false);
				return p;
			}
			case MessageType::UPDATE_CONFIG: {
				UpdateConfigPayload p;
				if (j.contains("config")) {
					p.config = LobbyConfig::deserialize(j["config"]);
				}
				return p;
			}
			case MessageType::START_GAME:
				return StartGamePayload{};
			case MessageType::KICK_PLAYER: {
				KickPlayerPayload p;
				p.playerName = j.value("playerName", "");
				return p;
			}
			case MessageType::END_TURN:
				return EndTurnPayload{};
			case MessageType::BUILD_SETTLEMENT: {
				BuildSettlementPayload p;
				p.vertexId = j.value("vertexId", size_t{0});
				return p;
			}
			case MessageType::BUILD_ROAD: {
				BuildRoadPayload p;
				p.edgeId = j.value("edgeId", size_t{0});
				p.level = stringToRoadLevel(j.value("level", "Path"));
				return p;
			}
			case MessageType::MOVE_HERO: {
				MoveHeroPayload p;
				p.targetTileId = j.value("targetTileId", size_t{0});
				return p;
			}
			case MessageType::PING: {
				PingPayload p;
				p.timestamp = j.value("timestamp", int64_t{0});
				return p;
			}
			case MessageType::RECONNECT: {
				ReconnectPayload p;
				p.playerName = j.value("playerName", "");
				return p;
			}
			case MessageType::JOIN_RESPONSE: {
				JoinResponsePayload p;
				p.success = j.value("success", false);
				p.playerId = j.value("playerId", size_t{0});
				if (j.contains("error") && !j["error"].is_null()) {
					p.error = ErrorInfo::deserialize(j["error"]);
				}
				return p;
			}
			case MessageType::LOBBY_STATE: {
				LobbyStatePayload p;
				p.lobby = LobbyState::deserialize(j);
				return p;
			}
			case MessageType::CONFIG_UPDATE: {
				ConfigUpdatePayload p;
				if (j.contains("config")) {
					p.config = LobbyConfig::deserialize(j["config"]);
				}
				return p;
			}
			case MessageType::KICKED: {
				KickedPayload p;
				p.reason = j.value("reason", "");
				return p;
			}
			case MessageType::GAME_STARTED: {
				GameStartedPayload p;
				if (j.contains("initialState")) {
					p.initialState = j["initialState"];
				}
				return p;
			}
			case MessageType::GAME_STATE: {
				GameStatePayload p;
				if (j.contains("state")) {
					p.state = j["state"];
				}
				return p;
			}
			case MessageType::ACTION_RESULT: {
				ActionResultPayload p;
				p.seq = j.value("seq", uint32_t{0});
				p.success = j.value("success", false);
				if (j.contains("error") && !j["error"].is_null()) {
					p.error = ErrorInfo::deserialize(j["error"]);
				}
				return p;
			}
			case MessageType::GAME_PAUSED: {
				GamePausedPayload p;
				p.disconnectedPlayer = j.value("disconnectedPlayer", "");
				p.timeoutSeconds = j.value("timeoutSeconds", DEFAULT_RECONNECT_TIMEOUT_SECONDS);
				return p;
			}
			case MessageType::GAME_RESUMED: {
				GameResumedPayload p;
				p.reconnectedPlayer = j.value("reconnectedPlayer", "");
				return p;
			}
			case MessageType::GAME_OVER: {
				GameOverPayload p;
				p.winnerId = j.value("winnerId", size_t{0});
				if (j.contains("scores") && j["scores"].is_array()) {
					for (const auto& s : j["scores"]) {
						p.scores.emplace_back(
							s.value("playerId", size_t{0}),
							s.value("score", 0));
					}
				}
				return p;
			}
			case MessageType::PONG: {
				PongPayload p;
				p.timestamp = j.value("timestamp", int64_t{0});
				p.serverTime = j.value("serverTime", int64_t{0});
				return p;
			}
			case MessageType::RECONNECT_RESPONSE: {
				ReconnectResponsePayload p;
				p.success = j.value("success", false);
				p.playerId = j.value("playerId", size_t{0});
				if (j.contains("state") && !j["state"].is_null()) {
					p.state = j["state"];
				}
				if (j.contains("error") && !j["error"].is_null()) {
					p.error = ErrorInfo::deserialize(j["error"]);
				}
				return p;
			}
			case MessageType::SERVER_ERROR: {
				ServerErrorPayload p;
				p.code = stringToErrorCode(j.value("code", "INTERNAL_ERROR"));
				p.message = j.value("message", "");
				return p;
			}
			case MessageType::UNKNOWN:
				return ServerErrorPayload{ErrorCode::INVALID_MESSAGE, "Unknown message type"};
			}

			return ServerErrorPayload{ErrorCode::INVALID_MESSAGE, "Unknown message type"};
		}

	} // namespace detail


	// ═══════════════════════════════════════════════════════════════════════════════
	// PLAYERINFO SERIALIZATION
	// ═══════════════════════════════════════════════════════════════════════════════

	nlohmann::json PlayerInfo::serialize() const {
		return {
			{"playerId", playerId},
			{"name", name},
			{"ready", ready},
			{"isHost", isHost},
			{"connected", connected}};
	}

	PlayerInfo PlayerInfo::deserialize(const nlohmann::json& j) {
		PlayerInfo p;
		p.playerId = j.value("playerId", size_t{0});
		p.name = j.value("name", "");
		p.ready = j.value("ready", false);
		p.isHost = j.value("isHost", false);
		p.connected = j.value("connected", true);
		return p;
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// LOBBYCONFIG SERIALIZATION
	// ═══════════════════════════════════════════════════════════════════════════════

	nlohmann::json LobbyConfig::serialize() const {
		std::string modeStr = (generationMode == GenerationMode::INSULAR)
								  ? "INSULAR"
								  : "PERLIN";
		return {
			{"version", version},
			{"columns", columns},
			{"rows", rows},
			{"generationMode", modeStr},
			{"seed", seed},
			{"reconnectTimeoutSeconds", reconnectTimeoutSeconds}};
	}

	LobbyConfig LobbyConfig::deserialize(const nlohmann::json& j) {
		LobbyConfig lc;
		lc.version = j.value("version", PROTOCOL_VERSION);
		lc.columns = j.value("columns", uint32_t{24});
		lc.rows = j.value("rows", uint32_t{24});

		std::string modeStr = j.value("generationMode", "PERLIN");
		lc.generationMode = (modeStr == "INSULAR")
								? GenerationMode::INSULAR
								: GenerationMode::PERLIN;

		lc.seed = j.value("seed", uint32_t{0});
		lc.reconnectTimeoutSeconds = j.value("reconnectTimeoutSeconds", DEFAULT_RECONNECT_TIMEOUT_SECONDS);
		return lc;
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// LOBBYSTATE SERIALIZATION
	// ═══════════════════════════════════════════════════════════════════════════════

	nlohmann::json LobbyState::serialize() const {
		nlohmann::json playersJson = nlohmann::json::array();
		for (const auto& p : players) {
			playersJson.push_back(p.serialize());
		}

		return {
			{"players", playersJson},
			{"hostId", hostId},
			{"config", config.serialize()},
			{"minPlayers", minPlayers},
			{"maxPlayers", maxPlayers}};
	}

	LobbyState LobbyState::deserialize(const nlohmann::json& j) {
		LobbyState ls;
		ls.hostId = j.value("hostId", size_t{0});
		ls.minPlayers = j.value("minPlayers", MIN_PLAYERS);
		ls.maxPlayers = j.value("maxPlayers", MAX_PLAYERS);

		if (j.contains("players") && j["players"].is_array()) {
			for (const auto& pj : j["players"]) {
				ls.players.push_back(PlayerInfo::deserialize(pj));
			}
		}

		if (j.contains("config")) {
			ls.config = LobbyConfig::deserialize(j["config"]);
		}

		return ls;
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// BUILDINGCOSTS SERIALIZATION
	// ═══════════════════════════════════════════════════════════════════════════════

	std::vector<int> BuildingCosts::getSettlementCostVector() const {
		std::vector<int> costs(static_cast<size_t>(types::TileType::COUNT), 0);
		for (const auto& [type, amount] : settlementCost) {
			costs[static_cast<size_t>(type)] = amount;
		}
		return costs;
	}

	std::vector<int> BuildingCosts::getRoadCostVector(RoadLevel level) const {
		std::vector<int> costs(static_cast<size_t>(types::TileType::COUNT), 0);
		auto it = roadCosts.find(level);
		if (it != roadCosts.end()) {
			for (const auto& [type, amount] : it->second) {
				costs[static_cast<size_t>(type)] = amount;
			}
		}
		return costs;
	}

	nlohmann::json BuildingCosts::serialize() const {
		nlohmann::json settlementJson;
		for (const auto& [type, amount] : settlementCost) {
			settlementJson[detail::tileTypeToString(type)] = amount;
		}

		nlohmann::json roadsJson;
		for (const auto& [level, costs] : roadCosts) {
			nlohmann::json costJson;
			for (const auto& [type, amount] : costs) {
				costJson[detail::tileTypeToString(type)] = amount;
			}
			roadsJson[detail::roadLevelToString(level)] = costJson;
		}

		return {
			{"settlementCost", settlementJson},
			{"roadCost", roadsJson}};
	}

	BuildingCosts BuildingCosts::deserialize(const nlohmann::json& j) {
		BuildingCosts bc;
		bc.settlementCost.clear();
		bc.roadCosts.clear();

		if (j.contains("settlementCost") && j["settlementCost"].is_object()) {
			for (const auto& [key, value] : j["settlementCost"].items()) {
				bc.settlementCost[detail::stringToTileType(key)] = value.get<int>();
			}
		}

		if (j.contains("roadCost") && j["roadCost"].is_object()) {
			for (const auto& [levelStr, costs] : j["roadCost"].items()) {
				RoadLevel level = detail::stringToRoadLevel(levelStr);
				std::map<types::TileType, int> costMap;
				for (const auto& [typeStr, amount] : costs.items()) {
					costMap[detail::stringToTileType(typeStr)] = amount.get<int>();
				}
				bc.roadCosts[level] = costMap;
			}
		}

		return bc;
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// ERRORINFO SERIALIZATION
	// ═══════════════════════════════════════════════════════════════════════════════

	nlohmann::json ErrorInfo::serialize() const {
		return {
			{"code", errorCodeToString(code)},
			{"message", message}};
	}

	ErrorInfo ErrorInfo::deserialize(const nlohmann::json& j) {
		ErrorInfo e;
		e.code = stringToErrorCode(j.value("code", "INTERNAL_ERROR"));
		e.message = j.value("message", "");
		return e;
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// MESSAGE SERIALIZATION
	// ═══════════════════════════════════════════════════════════════════════════════

	nlohmann::json Message::serialize() const {
		nlohmann::json j = detail::serializePayload(type, payload);
		j["type"] = messageTypeToString(type);
		j["seq"] = seq;
		return j;
	}

	Message Message::deserialize(const nlohmann::json& j) {
		Message msg;
		msg.type = stringToMessageType(j.value("type", "Unknown"));
		msg.seq = j.value("seq", uint32_t{0});
		msg.payload = detail::deserializePayload(msg.type, j);
		return msg;
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// MESSAGE FRAMING
	// ═══════════════════════════════════════════════════════════════════════════════

	std::vector<uint8_t> frameMessage(const Message& msg) {
		std::string jsonStr = msg.serialize().dump();
		uint32_t length = static_cast<uint32_t>(jsonStr.size());

		// Sanity check
		if (length > MAX_MESSAGE_SIZE) {
			throw std::runtime_error("[Bifrost] Message too large: " + std::to_string(length) + " bytes");
		}

		std::vector<uint8_t> result;
		result.reserve(HEADER_SIZE + length);

		// Write length in big-endian (network byte order)
		uint32_t networkLength = htonl(length);
		const uint8_t* lengthBytes = reinterpret_cast<const uint8_t*>(&networkLength);
		result.insert(result.end(), lengthBytes, lengthBytes + HEADER_SIZE);

		// Write JSON payload
		result.insert(result.end(), jsonStr.begin(), jsonStr.end());

		return result;
	}


	bool readExact(int socket, void* buffer, size_t length) {
		uint8_t* buf = static_cast<uint8_t*>(buffer);
		size_t totalRead = 0;

		while (totalRead < length) {
			ssize_t bytesRead = recv(socket, buf + totalRead, length - totalRead, 0);

			if (bytesRead <= 0) {
				if (bytesRead == 0) {
					// Connection closed
					return false;
				}
				if (errno == EINTR) {
					// Interrupted, retry
					continue;
				}
				// Error
				return false;
			}

			totalRead += static_cast<size_t>(bytesRead);
		}

		return true;
	}


	std::optional<Message> readFramedMessage(int socket) {
		// Read 4-byte length header
		uint32_t networkLength = 0;
		if (!readExact(socket, &networkLength, HEADER_SIZE)) {
			return std::nullopt;
		}

		uint32_t length = ntohl(networkLength);

		// Sanity check
		if (length > MAX_MESSAGE_SIZE) {
			fmt::println("[Bifrost] Received message too large: {} bytes", length);
			return std::nullopt;
		}

		if (length == 0) {
			fmt::println("[Bifrost] Received empty message");
			return std::nullopt;
		}

		// Read JSON payload
		std::vector<char> buffer(length);
		if (!readExact(socket, buffer.data(), length)) {
			return std::nullopt;
		}

		// Parse JSON
		try {
			std::string jsonStr(buffer.begin(), buffer.end());
			nlohmann::json j = nlohmann::json::parse(jsonStr);
			return Message::deserialize(j);
		} catch (const nlohmann::json::exception& e) {
			fmt::println("[Bifrost] JSON parse error: {}", e.what());
			return std::nullopt;
		}
	}


	bool sendFramedMessage(int socket, const Message& msg) {
		try {
			std::vector<uint8_t> data = frameMessage(msg);

			size_t totalSent = 0;
			while (totalSent < data.size()) {
				ssize_t bytesSent = send(socket, data.data() + totalSent, data.size() - totalSent, 0);

				if (bytesSent <= 0) {
					if (bytesSent == 0) {
						return false;
					}
					if (errno == EINTR) {
						continue;
					}
					return false;
				}

				totalSent += static_cast<size_t>(bytesSent);
			}

			return true;
		} catch (const std::exception& e) {
			fmt::println("[Bifrost] Error sending message: {}", e.what());
			return false;
		}
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// UTILITY FUNCTIONS
	// ═══════════════════════════════════════════════════════════════════════════════

	int64_t currentTimestampMs() {
		auto now = std::chrono::system_clock::now();
		auto epoch = now.time_since_epoch();
		return std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
	}


	Message createErrorMessage(uint32_t seq, ErrorCode code, const std::string& message) {
		Message msg;
		msg.type = MessageType::SERVER_ERROR;
		msg.seq = seq;
		msg.payload = ServerErrorPayload{code, message};
		return msg;
	}


	Message createJoinResponse(uint32_t seq, bool success, size_t playerId, std::optional<ErrorInfo> error) {
		Message msg;
		msg.type = MessageType::JOIN_RESPONSE;
		msg.seq = seq;
		msg.payload = JoinResponsePayload{success, playerId, error};
		return msg;
	}


	Message createActionResult(uint32_t seq, bool success, std::optional<ErrorInfo> error) {
		Message msg;
		msg.type = MessageType::ACTION_RESULT;
		msg.seq = seq;
		msg.payload = ActionResultPayload{seq, success, error};
		return msg;
	}


	Message createLobbyStateMessage(uint32_t seq, const LobbyState& lobby) {
		Message msg;
		msg.type = MessageType::LOBBY_STATE;
		msg.seq = seq;
		msg.payload = LobbyStatePayload{lobby};
		return msg;
	}


	Message createGameStateMessage(uint32_t seq, const nlohmann::json& state) {
		Message msg;
		msg.type = MessageType::GAME_STATE;
		msg.seq = seq;
		msg.payload = GameStatePayload{state};
		return msg;
	}


	Message createPongMessage(uint32_t seq, int64_t clientTimestamp) {
		Message msg;
		msg.type = MessageType::PONG;
		msg.seq = seq;
		msg.payload = PongPayload{clientTimestamp, currentTimestampMs()};
		return msg;
	}


} // namespace df::bifrost
