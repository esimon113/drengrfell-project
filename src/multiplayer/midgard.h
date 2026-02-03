/**
 * @file midgard.h
 * @brief Client-side network handler for Bifrost multiplayer
 *
 * Midgard represents the client side of the Bifrost protocol.
 * It wraps the TcpClient with Bifrost protocol handling, providing a high-level
 * interface for game clients to communicate with the server.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>

#include "bifrost.h"
#include "multiplayer/network/tcpClient.h"


namespace df::bifrost {

	// ═══════════════════════════════════════════════════════════════════════════════
	// CALLBACK TYPES
	// ═══════════════════════════════════════════════════════════════════════════════

	/**
	 * Callback for lobby state updates.
	 */
	using LobbyStateCallback = std::function<void(const LobbyState&)>;

	/**
	 * Callback for game state updates.
	 */
	using GameStateCallback = std::function<void(const nlohmann::json&)>;

	/**
	 * Callback for action results.
	 */
	using ActionResultCallback = std::function<void(uint32_t seq, bool success, const std::optional<ErrorInfo>&)>;

	/**
	 * Callback for game events (paused, resumed, over).
	 */
	using GameEventCallback = std::function<void(const Message&)>;

	/**
	 * Callback for connection events.
	 */
	using ConnectionCallback = std::function<void(bool connected, const std::string& reason)>;

	/**
	 * Callback for being kicked.
	 */
	using KickedCallback = std::function<void(const std::string& reason)>;


	// ═══════════════════════════════════════════════════════════════════════════════
	// MIDGARD CLIENT
	// ═══════════════════════════════════════════════════════════════════════════════

	/**
	 * Client-side handler for Bifrost multiplayer protocol.
	 *
	 * Provides a high-level interface for:
	 * - Connecting/disconnecting from server
	 * - Joining/leaving sessions
	 * - Lobby operations (ready, config viewing)
	 * - Game actions (build, move, end turn)
	 * - Receiving state updates
	 */
	class Midgard {
	  public:
		Midgard();
		~Midgard();

		// Disable copy/move
		Midgard(const Midgard&) = delete;
		Midgard& operator=(const Midgard&) = delete;
		Midgard(Midgard&&) = delete;
		Midgard& operator=(Midgard&&) = delete;

		// ─────────────────────────────────────────────────────────────────────────
		// CONNECTION
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Connect to a game server.
		 *
		 * @param host Server hostname or IP
		 * @param port Server port
		 * @return true if connection successful
		 */
		bool connect(const std::string& host, uint16_t port = DEFAULT_PORT);

		/**
		 * Disconnect from the server.
		 */
		void disconnect();

		/**
		 * Check if connected to server.
		 */
		[[nodiscard]] bool isConnected() const;

		// ─────────────────────────────────────────────────────────────────────────
		// CALLBACKS
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Set callback for lobby state updates.
		 */
		void setLobbyStateCallback(LobbyStateCallback callback);

		/**
		 * Set callback for game state updates.
		 */
		void setGameStateCallback(GameStateCallback callback);

		/**
		 * Set callback for action results.
		 */
		void setActionResultCallback(ActionResultCallback callback);

		/**
		 * Set callback for game events (pause, resume, game over).
		 */
		void setGameEventCallback(GameEventCallback callback);

		/**
		 * Set callback for connection events.
		 */
		void setConnectionCallback(ConnectionCallback callback);

		/**
		 * Set callback for being kicked.
		 */
		void setKickedCallback(KickedCallback callback);

		// ─────────────────────────────────────────────────────────────────────────
		// LOBBY ACTIONS
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Join the game session with a player name.
		 *
		 * @param playerName Name to join with
		 * @return true if join request sent successfully
		 */
		bool join(const std::string& playerName);

		/**
		 * Leave the current session.
		 */
		void leave();

		/**
		 * Set ready state.
		 *
		 * @param ready Whether the player is ready
		 */
		void setReady(bool ready);

		/**
		 * Update lobby config (host only).
		 *
		 * @param config New lobby configuration
		 */
		void updateConfig(const LobbyConfig& config);

		/**
		 * Start the game (host only).
		 */
		void startGame();

		/**
		 * Kick a player (host only).
		 *
		 * @param playerName Name of player to kick
		 */
		void kickPlayer(const std::string& playerName);

		/**
		 * Attempt to reconnect after disconnect.
		 *
		 * @param playerName Name used in previous session
		 */
		void reconnect(const std::string& playerName);

		// ─────────────────────────────────────────────────────────────────────────
		// GAME ACTIONS
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * End the current turn.
		 */
		void endTurn();

		/**
		 * Build a settlement at a vertex.
		 *
		 * @param vertexId Vertex ID to build at
		 */
		void buildSettlement(size_t vertexId);

		/**
		 * Build a road at an edge.
		 *
		 * @param edgeId Edge ID to build at
		 * @param level Road level
		 */
		void buildRoad(size_t edgeId, RoadLevel level = RoadLevel::Path);

		/**
		 * Move hero to a tile.
		 *
		 * @param targetTileId Target tile ID
		 */
		void moveHero(size_t targetTileId);

		/**
		 * Send a ping to measure latency.
		 */
		void ping();

		// ─────────────────────────────────────────────────────────────────────────
		// STATE QUERIES
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Get assigned player ID.
		 */
		[[nodiscard]] std::optional<size_t> getPlayerId() const;

		/**
		 * Get current player name.
		 */
		[[nodiscard]] const std::string& getPlayerName() const;

		/**
		 * Get last known lobby state.
		 */
		[[nodiscard]] std::optional<LobbyState> getLastLobbyState() const;

		/**
		 * Get last known game state.
		 */
		[[nodiscard]] std::optional<nlohmann::json> getLastGameState() const;

		/**
		 * Check if we are the host.
		 */
		[[nodiscard]] bool isHost() const;

		/**
		 * Get last measured latency in milliseconds.
		 */
		[[nodiscard]] int64_t getLatencyMs() const;


	  private:
		// ─────────────────────────────────────────────────────────────────────────
		// PRIVATE MEMBERS
		// ─────────────────────────────────────────────────────────────────────────

		mutable std::mutex mutex_;

		// Network
		mp::TcpClient tcpClient_;
		std::atomic<bool> connected_{false};
		std::atomic<bool> shouldStop_{false};
		std::unique_ptr<std::jthread> receiveThread_;

		// Player state
		std::optional<size_t> playerId_;
		std::string playerName_;
		bool isHost_{false};

		// Cached state
		std::optional<LobbyState> lastLobbyState_;
		std::optional<nlohmann::json> lastGameState_;

		// Sequence number for requests
		std::atomic<uint32_t> nextSeq_{1};

		// Latency tracking
		std::atomic<int64_t> lastLatencyMs_{0};
		std::atomic<int64_t> lastPingTimestamp_{0};

		// Callbacks
		LobbyStateCallback lobbyStateCallback_;
		GameStateCallback gameStateCallback_;
		ActionResultCallback actionResultCallback_;
		GameEventCallback gameEventCallback_;
		ConnectionCallback connectionCallback_;
		KickedCallback kickedCallback_;

		// ─────────────────────────────────────────────────────────────────────────
		// PRIVATE METHODS
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Send a message to the server.
		 */
		bool sendMessage(const Message& msg);

		/**
		 * Get next sequence number.
		 */
		uint32_t getNextSeq();

		/**
		 * Receive loop running in background thread.
		 */
		void receiveLoop(std::stop_token stopToken);

		/**
		 * Handle a received message.
		 */
		void handleMessage(const Message& msg);

		/**
		 * Handle connection lost.
		 */
		void handleDisconnect(const std::string& reason);
	};

} // namespace df::bifrost

