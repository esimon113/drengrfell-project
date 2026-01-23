/**
 * @file bifrost.h
 * @brief Bifrost Protocol v1.0 - Core protocol definitions for Drengrfell multiplayer
 *
 * In Norse mythology, "Bifrost" is the rainbow bridge connecting Asgard
 * with Midgard. This protocol connects game clients to the server.
 *
 * Design principles:
 * - Server Authoritative: Server owns the single source of truth (GameState)
 * - Intent-Based Commands: Clients send what they want to do, not what happened
 * - Full State Sync: Complete GameState broadcast after every action
 * - No Client Prediction: Clients wait for server confirmation
 */

#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

// Forward declarations to avoid heavy dependencies
// These are lightweight includes that don't pull in OpenGL
#include "../core/types.h"
#include "../core/worldGeneratorConfig.h"


namespace df::bifrost {

	// ═══════════════════════════════════════════════════════════════════════════════
	// LOCAL TYPE DEFINITIONS (to avoid heavy dependencies)
	// ═══════════════════════════════════════════════════════════════════════════════

	/**
	 * Road level enum (mirrors df::RoadLevel from road.h)
	 */
	enum class RoadLevel : size_t {
		Path = 0,
		DirtRoad,
		StoneRoad,
		HighQualityRoad
	};

	/**
	 * World generation mode (mirrors df::WorldGeneratorConfig::GenerationMode)
	 */
	enum class GenerationMode {
		INSULAR, // Like in the classic board game
		PERLIN	 // Perlin noise based
	};


	// ═══════════════════════════════════════════════════════════════════════════════
	// SECTION 1: PROTOCOL CONSTANTS
	// ═══════════════════════════════════════════════════════════════════════════════

	inline constexpr uint32_t PROTOCOL_VERSION = 1;
	inline constexpr uint32_t MAX_MESSAGE_SIZE = 16 * 1024 * 1024; // 16 MB
	inline constexpr uint32_t HEADER_SIZE = 4;					   // 4 bytes for length prefix
	inline constexpr size_t MIN_PLAYERS = 2;
	inline constexpr size_t MAX_PLAYERS = 6;
	inline constexpr uint32_t DEFAULT_RECONNECT_TIMEOUT_SECONDS = 60;
	inline constexpr uint16_t DEFAULT_PORT = 7777;


	// ═══════════════════════════════════════════════════════════════════════════════
	// SECTION 2: ERROR CODES
	// ═══════════════════════════════════════════════════════════════════════════════

	enum class ErrorCode {
		NONE = 0,
		INVALID_MESSAGE,
		NOT_YOUR_TURN,
		INVALID_ACTION,
		INSUFFICIENT_RESOURCES,
		INVALID_LOCATION,
		LOBBY_FULL,
		NAME_TAKEN,
		NOT_HOST,
		GAME_ALREADY_STARTED,
		NOT_ALL_READY,
		PLAYER_NOT_FOUND,
		CONNECTION_LOST,
		TIMEOUT,
		INTERNAL_ERROR
	};

	[[nodiscard]] std::string errorCodeToString(ErrorCode code);
	[[nodiscard]] ErrorCode stringToErrorCode(const std::string& str);


	// ═══════════════════════════════════════════════════════════════════════════════
	// SECTION 3: MESSAGE TYPES
	// ═══════════════════════════════════════════════════════════════════════════════

	enum class MessageType {
		// Client -> Server: Lobby
		JOIN_REQUEST,
		LEAVE_REQUEST,
		READY_TOGGLE,
		UPDATE_CONFIG,
		START_GAME,
		KICK_PLAYER,

		// Client -> Server: Game
		END_TURN,
		BUILD_SETTLEMENT,
		BUILD_ROAD,
		MOVE_HERO,

		// Client -> Server: Connection
		PING,
		RECONNECT,

		// Server -> Client: Lobby
		JOIN_RESPONSE,
		LOBBY_STATE,
		CONFIG_UPDATE,
		KICKED,

		// Server -> Client: Game
		GAME_STARTED,
		GAME_STATE,
		ACTION_RESULT,
		GAME_PAUSED,
		GAME_RESUMED,
		GAME_OVER,

		// Server -> Client: Connection
		PONG,
		RECONNECT_RESPONSE,
		SERVER_ERROR,

		UNKNOWN
	};

	[[nodiscard]] std::string messageTypeToString(MessageType type);
	[[nodiscard]] MessageType stringToMessageType(const std::string& str);


	// ═══════════════════════════════════════════════════════════════════════════════
	// SECTION 4: SESSION STATE
	// ═══════════════════════════════════════════════════════════════════════════════

	enum class SessionState {
		LOBBY,
		PLAYING,
		PAUSED,
		ENDED
	};


	// ═══════════════════════════════════════════════════════════════════════════════
	// SECTION 5: PLAYER INFO & LOBBY STATE
	// ═══════════════════════════════════════════════════════════════════════════════

	struct PlayerInfo {
		size_t playerId{0};
		std::string name;
		bool ready{false};
		bool isHost{false};
		bool connected{true};

		[[nodiscard]] nlohmann::json serialize() const;
		static PlayerInfo deserialize(const nlohmann::json& j);
	};


	struct LobbyConfig {
		uint32_t version{PROTOCOL_VERSION};
		uint32_t columns{24};
		uint32_t rows{24};
		GenerationMode generationMode{GenerationMode::PERLIN};
		uint32_t seed{0};
		uint32_t reconnectTimeoutSeconds{DEFAULT_RECONNECT_TIMEOUT_SECONDS};

		[[nodiscard]] nlohmann::json serialize() const;
		static LobbyConfig deserialize(const nlohmann::json& j);

		/**
		 * Convert to WorldGeneratorConfig for game initialization.
		 */
		[[nodiscard]] df::WorldGeneratorConfig toWorldGeneratorConfig() const {
			df::WorldGeneratorConfig wgc;
			wgc.version = version;
			wgc.columns = columns;
			wgc.rows = rows;
			wgc.generationMode = (generationMode == GenerationMode::INSULAR)
				? df::WorldGeneratorConfig::GenerationMode::INSULAR
				: df::WorldGeneratorConfig::GenerationMode::PERLIN;
			wgc.seed = seed;
			return wgc;
		}
	};


	struct LobbyState {
		std::vector<PlayerInfo> players;
		size_t hostId{0};
		LobbyConfig config;
		size_t minPlayers{MIN_PLAYERS};
		size_t maxPlayers{MAX_PLAYERS};

		[[nodiscard]] nlohmann::json serialize() const;
		static LobbyState deserialize(const nlohmann::json& j);
	};


	// ═══════════════════════════════════════════════════════════════════════════════
	// SECTION 6: BUILDING COSTS CONFIGURATION
	// ═══════════════════════════════════════════════════════════════════════════════

	struct BuildingCosts {
		// Settlement cost: map of TileType -> amount
		std::map<types::TileType, int> settlementCost{
			{types::TileType::FOREST, 1},
			{types::TileType::CLAY, 1},
			{types::TileType::GRASS, 1},
			{types::TileType::FIELD, 1}};

		// Road costs per level
		std::map<RoadLevel, std::map<types::TileType, int>> roadCosts{
			{RoadLevel::Path, {{types::TileType::FOREST, 1}, {types::TileType::CLAY, 1}}},
			{RoadLevel::DirtRoad, {{types::TileType::FOREST, 2}, {types::TileType::CLAY, 2}}},
			{RoadLevel::StoneRoad, {{types::TileType::FOREST, 2}, {types::TileType::CLAY, 2}, {types::TileType::MOUNTAIN, 1}}},
			{RoadLevel::HighQualityRoad, {{types::TileType::FOREST, 3}, {types::TileType::CLAY, 3}, {types::TileType::MOUNTAIN, 2}}}};

		// Convert to vector format used by GameController
		[[nodiscard]] std::vector<int> getSettlementCostVector() const;
		[[nodiscard]] std::vector<int> getRoadCostVector(RoadLevel level) const;

		[[nodiscard]] nlohmann::json serialize() const;
		static BuildingCosts deserialize(const nlohmann::json& j);
	};


	// ═══════════════════════════════════════════════════════════════════════════════
	// SECTION 7: ERROR INFO
	// ═══════════════════════════════════════════════════════════════════════════════

	struct ErrorInfo {
		ErrorCode code{ErrorCode::NONE};
		std::string message;

		[[nodiscard]] bool hasError() const { return code != ErrorCode::NONE; }

		[[nodiscard]] nlohmann::json serialize() const;
		static ErrorInfo deserialize(const nlohmann::json& j);
	};


	// ═══════════════════════════════════════════════════════════════════════════════
	// SECTION 8: MESSAGE PAYLOADS
	// ═══════════════════════════════════════════════════════════════════════════════

	// --- Client -> Server Payloads ---

	struct JoinRequestPayload {
		std::string playerName;
	};

	struct LeaveRequestPayload {};

	struct ReadyTogglePayload {
		bool ready{false};
	};

	struct UpdateConfigPayload {
		LobbyConfig config;
	};

	struct StartGamePayload {};

	struct KickPlayerPayload {
		std::string playerName;
	};

	struct EndTurnPayload {};

	struct BuildSettlementPayload {
		size_t vertexId{0};
	};

	struct BuildRoadPayload {
		size_t edgeId{0};
		RoadLevel level{RoadLevel::Path};
	};

	struct MoveHeroPayload {
		size_t targetTileId{0};
	};

	struct PingPayload {
		int64_t timestamp{0};
	};

	struct ReconnectPayload {
		std::string playerName;
	};


	// --- Server -> Client Payloads ---

	struct JoinResponsePayload {
		bool success{false};
		size_t playerId{0};
		std::optional<ErrorInfo> error;
	};

	struct LobbyStatePayload {
		LobbyState lobby;
	};

	struct ConfigUpdatePayload {
		LobbyConfig config;
	};

	struct KickedPayload {
		std::string reason;
	};

	struct GameStartedPayload {
		nlohmann::json initialState; // Serialized GameState
	};

	struct GameStatePayload {
		nlohmann::json state; // Serialized GameState
	};

	struct ActionResultPayload {
		uint32_t seq{0};
		bool success{false};
		std::optional<ErrorInfo> error;
	};

	struct GamePausedPayload {
		std::string disconnectedPlayer;
		uint32_t timeoutSeconds{DEFAULT_RECONNECT_TIMEOUT_SECONDS};
	};

	struct GameResumedPayload {
		std::string reconnectedPlayer;
	};

	struct GameOverPayload {
		size_t winnerId{0};
		std::vector<std::pair<size_t, int>> scores; // playerId -> score
	};

	struct PongPayload {
		int64_t timestamp{0};
		int64_t serverTime{0};
	};

	struct ReconnectResponsePayload {
		bool success{false};
		size_t playerId{0};
		std::optional<nlohmann::json> state; // Serialized GameState if success
		std::optional<ErrorInfo> error;
	};

	struct ServerErrorPayload {
		ErrorCode code{ErrorCode::INTERNAL_ERROR};
		std::string message;
	};


	// ═══════════════════════════════════════════════════════════════════════════════
	// SECTION 9: UNIFIED MESSAGE STRUCTURE
	// ═══════════════════════════════════════════════════════════════════════════════

	using MessagePayload = std::variant<
		// Client -> Server
		JoinRequestPayload,
		LeaveRequestPayload,
		ReadyTogglePayload,
		UpdateConfigPayload,
		StartGamePayload,
		KickPlayerPayload,
		EndTurnPayload,
		BuildSettlementPayload,
		BuildRoadPayload,
		MoveHeroPayload,
		PingPayload,
		ReconnectPayload,
		// Server -> Client
		JoinResponsePayload,
		LobbyStatePayload,
		ConfigUpdatePayload,
		KickedPayload,
		GameStartedPayload,
		GameStatePayload,
		ActionResultPayload,
		GamePausedPayload,
		GameResumedPayload,
		GameOverPayload,
		PongPayload,
		ReconnectResponsePayload,
		ServerErrorPayload>;


	struct Message {
		MessageType type{MessageType::UNKNOWN};
		uint32_t seq{0};
		MessagePayload payload;

		[[nodiscard]] nlohmann::json serialize() const;
		static Message deserialize(const nlohmann::json& j);
	};


	// ═══════════════════════════════════════════════════════════════════════════════
	// SECTION 10: MESSAGE FRAMING (LENGTH-PREFIX)
	// ═══════════════════════════════════════════════════════════════════════════════

	/**
	 * Frame a message for transmission over TCP.
	 * Format: [4 bytes length (big-endian)] [JSON payload]
	 *
	 * @param msg The message to frame
	 * @return Framed bytes ready for transmission
	 */
	[[nodiscard]] std::vector<uint8_t> frameMessage(const Message& msg);

	/**
	 * Read a framed message from a socket.
	 * Blocks until a complete message is received or an error occurs.
	 *
	 * @param socket The socket file descriptor
	 * @return The parsed message, or std::nullopt on error/disconnect
	 */
	[[nodiscard]] std::optional<Message> readFramedMessage(int socket);

	/**
	 * Send a framed message to a socket.
	 *
	 * @param socket The socket file descriptor
	 * @param msg The message to send
	 * @return true on success, false on error
	 */
	bool sendFramedMessage(int socket, const Message& msg);

	/**
	 * Read exactly n bytes from a socket.
	 * Blocks until all bytes are received or an error occurs.
	 *
	 * @param socket The socket file descriptor
	 * @param buffer The buffer to read into
	 * @param length The number of bytes to read
	 * @return true on success, false on error/disconnect
	 */
	bool readExact(int socket, void* buffer, size_t length);


	// ═══════════════════════════════════════════════════════════════════════════════
	// SECTION 11: UTILITY FUNCTIONS
	// ═══════════════════════════════════════════════════════════════════════════════

	/**
	 * Get current timestamp in milliseconds since epoch.
	 */
	[[nodiscard]] int64_t currentTimestampMs();

	/**
	 * Create a simple error message.
	 */
	[[nodiscard]] Message createErrorMessage(uint32_t seq, ErrorCode code, const std::string& message);

	/**
	 * Create common response messages.
	 */
	[[nodiscard]] Message createJoinResponse(uint32_t seq, bool success, size_t playerId,
											 std::optional<ErrorInfo> error = std::nullopt);
	[[nodiscard]] Message createActionResult(uint32_t seq, bool success,
											 std::optional<ErrorInfo> error = std::nullopt);
	[[nodiscard]] Message createLobbyStateMessage(uint32_t seq, const LobbyState& lobby);
	[[nodiscard]] Message createGameStateMessage(uint32_t seq, const nlohmann::json& state);
	[[nodiscard]] Message createPongMessage(uint32_t seq, int64_t clientTimestamp);


	// ═══════════════════════════════════════════════════════════════════════════════
	// SECTION 12: JSON SERIALIZATION HELPERS
	// ═══════════════════════════════════════════════════════════════════════════════

	namespace detail {

		// Serialize payload to JSON based on message type
		[[nodiscard]] nlohmann::json serializePayload(MessageType type, const MessagePayload& payload);

		// Deserialize payload from JSON based on message type
		[[nodiscard]] MessagePayload deserializePayload(MessageType type, const nlohmann::json& j);

		// TileType string conversion
		[[nodiscard]] std::string tileTypeToString(types::TileType type);
		[[nodiscard]] types::TileType stringToTileType(const std::string& str);

		// RoadLevel conversion
		[[nodiscard]] std::string roadLevelToString(bifrost::RoadLevel level);
		[[nodiscard]] bifrost::RoadLevel stringToRoadLevel(const std::string& str);

	} // namespace detail

} // namespace df::bifrost
