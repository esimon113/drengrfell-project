/**
 * @file sessionManager.h
 * @brief Server-side game session management for Bifrost multiplayer
 *
 * The SessionManager owns the authoritative GameState and GameController,
 * manages connected clients, validates actions, and broadcasts state updates.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "../core/gamecontroller.h"
#include "../core/gamestate.h"
#include "../registry.h"
#include "bifrost.h"


namespace df::bifrost {

	// ═══════════════════════════════════════════════════════════════════════════════
	// CONNECTED CLIENT INFO
	// ═══════════════════════════════════════════════════════════════════════════════

	/**
	 * Represents a connected client from the server's perspective.
	 */
	struct ConnectedClient {
		int socket{-1};
		size_t playerId{0};
		std::string playerName;
		bool ready{false};
		bool isHost{false};
		bool connected{true};

		// Disconnect tracking
		std::chrono::steady_clock::time_point disconnectTime;
	};


	// ═══════════════════════════════════════════════════════════════════════════════
	// SESSION MANAGER
	// ═══════════════════════════════════════════════════════════════════════════════

	/**
	 * Manages a single multiplayer game session.
	 *
	 * Responsibilities:
	 * - Owns the authoritative GameState and GameController
	 * - Manages connected clients and their states
	 * - Validates and executes game actions
	 * - Broadcasts state updates to all clients
	 * - Handles reconnection within timeout
	 */
	class SessionManager {
	  public:
		SessionManager();
		~SessionManager();

		// Disable copy/move
		SessionManager(const SessionManager&) = delete;
		SessionManager& operator=(const SessionManager&) = delete;
		SessionManager(SessionManager&&) = delete;
		SessionManager& operator=(SessionManager&&) = delete;

		// ─────────────────────────────────────────────────────────────────────────
		// SESSION LIFECYCLE
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Reset the session to initial state (lobby).
		 */
		void reset();

		/**
		 * Get current session state.
		 */
		[[nodiscard]] SessionState getState() const;

		/**
		 * Check if game can be started.
		 */
		[[nodiscard]] bool canStartGame() const;

		// ─────────────────────────────────────────────────────────────────────────
		// CLIENT MANAGEMENT
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Handle a new client connection.
		 * Returns the assigned player ID if successful, or nullopt if rejected.
		 */
		[[nodiscard]] std::optional<size_t> addClient(int socket, const std::string& playerName);

		/**
		 * Remove a client (voluntary leave or kick).
		 */
		void removeClient(int socket);

		/**
		 * Mark a client as disconnected (connection lost).
		 * Starts reconnect timeout if in game.
		 */
		void markClientDisconnected(int socket);

		/**
		 * Handle reconnection attempt.
		 * Returns player ID if successful.
		 */
		[[nodiscard]] std::optional<size_t> handleReconnect(int socket, const std::string& playerName);

		/**
		 * Get client by socket.
		 */
		[[nodiscard]] ConnectedClient* getClientBySocket(int socket);
		[[nodiscard]] const ConnectedClient* getClientBySocket(int socket) const;

		/**
		 * Get client by player name.
		 */
		[[nodiscard]] ConnectedClient* getClientByName(const std::string& name);

		/**
		 * Get all connected client sockets.
		 */
		[[nodiscard]] std::vector<int> getAllClientSockets() const;

		/**
		 * Get count of connected players.
		 */
		[[nodiscard]] size_t getConnectedPlayerCount() const;

		/**
		 * Check if all players are ready.
		 */
		[[nodiscard]] bool areAllPlayersReady() const;

		// ─────────────────────────────────────────────────────────────────────────
		// MESSAGE HANDLING
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Process an incoming message from a client.
		 * Returns a response message (may be error), plus optional broadcast flag.
		 */
		struct MessageResult {
			Message response;
			bool broadcastLobby{false};
			bool broadcastGameState{false};
			std::optional<Message> additionalBroadcast;
		};

		[[nodiscard]] MessageResult processMessage(int socket, const Message& msg);

		// ─────────────────────────────────────────────────────────────────────────
		// LOBBY OPERATIONS
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Set player ready state.
		 */
		void setPlayerReady(int socket, bool ready);

		/**
		 * Update lobby config (host only).
		 */
		[[nodiscard]] bool updateConfig(int socket, const LobbyConfig& config);

		/**
		 * Kick a player (host only).
		 */
		[[nodiscard]] bool kickPlayer(int socket, const std::string& playerName);

		/**
		 * Start the game (host only, all players must be ready).
		 */
		[[nodiscard]] bool startGame(int socket);

		/**
		 * Get current lobby state.
		 */
		[[nodiscard]] LobbyState getLobbyState() const;

		/**
		 * Get current lobby config.
		 */
		[[nodiscard]] const LobbyConfig& getConfig() const;

		// ─────────────────────────────────────────────────────────────────────────
		// GAME OPERATIONS
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * End current player's turn.
		 */
		[[nodiscard]] bool endTurn(int socket);

		/**
		 * Build a settlement.
		 */
		[[nodiscard]] std::pair<bool, std::optional<ErrorInfo>> buildSettlement(int socket, size_t vertexId);

		/**
		 * Build a road.
		 */
		[[nodiscard]] std::pair<bool, std::optional<ErrorInfo>> buildRoad(int socket, size_t edgeId, RoadLevel level);

		/**
		 * Move hero (stub for now).
		 */
		[[nodiscard]] std::pair<bool, std::optional<ErrorInfo>> moveHero(int socket, size_t targetTileId);

		/**
		 * Get serialized game state.
		 */
		[[nodiscard]] nlohmann::json getSerializedGameState() const;

		/**
		 * Check if it's the given player's turn.
		 */
		[[nodiscard]] bool isPlayersTurn(int socket) const;

		// ─────────────────────────────────────────────────────────────────────────
		// PAUSE/RECONNECT
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Check for reconnect timeouts and kick players who exceeded timeout.
		 * Returns list of kicked player names.
		 */
		[[nodiscard]] std::vector<std::string> checkReconnectTimeouts();

		/**
		 * Check if any player is disconnected (game should be paused).
		 */
		[[nodiscard]] bool hasDisconnectedPlayers() const;

		/**
		 * Get name of first disconnected player (for GamePaused message).
		 */
		[[nodiscard]] std::string getFirstDisconnectedPlayerName() const;

		// ─────────────────────────────────────────────────────────────────────────
		// BUILDING COSTS
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Get building costs configuration.
		 */
		[[nodiscard]] const BuildingCosts& getBuildingCosts() const;

		/**
		 * Set building costs configuration.
		 */
		void setBuildingCosts(const BuildingCosts& costs);


	  private:
		// ─────────────────────────────────────────────────────────────────────────
		// PRIVATE MEMBERS
		// ─────────────────────────────────────────────────────────────────────────

		mutable std::mutex mutex_;

		SessionState state_{SessionState::LOBBY};

		// Client management
		std::unordered_map<int, ConnectedClient> clients_; // socket -> client
		size_t nextPlayerId_{0};
		std::optional<int> hostSocket_;

		// Lobby config
		LobbyConfig config_;
		BuildingCosts buildingCosts_;

		// Game state (owned by session)
		std::unique_ptr<Registry> registry_;
		std::unique_ptr<GameState> gameState_;
		std::unique_ptr<GameController> gameController_;

		// Server-owned RNG
		std::mt19937 rng_;

		// ─────────────────────────────────────────────────────────────────────────
		// PRIVATE METHODS
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Find player ID by socket.
		 */
		[[nodiscard]] std::optional<size_t> getPlayerIdBySocket(int socket) const;

		/**
		 * Check if socket belongs to host.
		 */
		[[nodiscard]] bool isHost(int socket) const;

		/**
		 * Initialize game state for a new game.
		 */
		void initializeGame();

		/**
		 * Create Player objects in GameState for all connected clients.
		 */
		void createPlayers();

		/**
		 * Validate that an action can be performed in current state.
		 */
		[[nodiscard]] std::optional<ErrorInfo> validateAction(int socket, MessageType action) const;
	};

} // namespace df::bifrost

