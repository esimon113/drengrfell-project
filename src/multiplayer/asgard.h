/**
 * @file asgard.h
 * @brief Server launcher for Bifrost multiplayer
 *
 * Asgard represents the authoritative game server.
 * It wraps the TcpServer with Bifrost protocol handling and integrates
 * the SessionManager for game logic.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "bifrost.h"
#include "sessionManager.h"
#include "multiplayer/network/tcpServer.h"


namespace df::bifrost {

	// ═══════════════════════════════════════════════════════════════════════════════
	// ASGARD SERVER
	// ═══════════════════════════════════════════════════════════════════════════════

	/**
	 * Authoritative game server for Bifrost multiplayer.
	 *
	 * Responsibilities:
	 * - Manages the TcpServer for network I/O
	 * - Integrates SessionManager for game logic
	 * - Routes messages between clients and session
	 * - Broadcasts state updates to all clients
	 * - Handles client connections/disconnections
	 */
	class Asgard {
	  public:
		Asgard();
		~Asgard();

		// Disable copy/move
		Asgard(const Asgard&) = delete;
		Asgard& operator=(const Asgard&) = delete;
		Asgard(Asgard&&) = delete;
		Asgard& operator=(Asgard&&) = delete;

		// ─────────────────────────────────────────────────────────────────────────
		// CONFIGURATION
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Configure the server before starting.
		 *
		 * @param port Port to listen on
		 * @param bindAddress Address to bind to (default: all interfaces)
		 */
		void configure(uint16_t port = DEFAULT_PORT, const std::string& bindAddress = "0.0.0.0");

		/**
		 * Set maximum number of connections.
		 */
		void setMaxConnections(size_t maxConnections);

		/**
		 * Set building costs for the session.
		 */
		void setBuildingCosts(const BuildingCosts& costs);

		/**
		 * Set initial lobby configuration.
		 */
		void setInitialConfig(const LobbyConfig& config);

		// ─────────────────────────────────────────────────────────────────────────
		// SERVER LIFECYCLE
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Start the server.
		 * This call returns immediately; the server runs in the background.
		 */
		void start();

		/**
		 * Start the server and block until stopped.
		 */
		void run();

		/**
		 * Stop the server.
		 */
		void stop();

		/**
		 * Check if server is running.
		 */
		[[nodiscard]] bool isRunning() const;

		// ─────────────────────────────────────────────────────────────────────────
		// STATE ACCESS
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Get the session manager.
		 * Use this to query game state or configure the session.
		 */
		[[nodiscard]] SessionManager& getSession();
		[[nodiscard]] const SessionManager& getSession() const;

		/**
		 * Get current lobby state.
		 */
		[[nodiscard]] LobbyState getLobbyState() const;

		/**
		 * Get number of connected players.
		 */
		[[nodiscard]] size_t getConnectedPlayerCount() const;


	  private:
		// ─────────────────────────────────────────────────────────────────────────
		// PRIVATE MEMBERS
		// ─────────────────────────────────────────────────────────────────────────

		mutable std::mutex mutex_;

		uint16_t port_{DEFAULT_PORT};
		std::string bindAddress_{"0.0.0.0"};
		size_t maxConnections_{MAX_PLAYERS};

		std::atomic<bool> running_{false};
		std::unique_ptr<std::jthread> serverThread_;
		std::unique_ptr<std::jthread> timeoutThread_;

		// Session manager owns the game state
		SessionManager session_;

		// Track connected sockets for broadcasting
		mutable std::mutex socketsMutex_;
		std::unordered_map<int, std::string> connectedSockets_; // socket -> player name

		// ─────────────────────────────────────────────────────────────────────────
		// PRIVATE METHODS
		// ─────────────────────────────────────────────────────────────────────────

		/**
		 * Handle a client connection.
		 * Called by TcpServer for each connected client.
		 */
		void handleClient(int socket, std::stop_token stopToken);

		/**
		 * Broadcast a message to all connected clients.
		 */
		void broadcast(const Message& msg);

		/**
		 * Broadcast a message to all clients except one.
		 */
		void broadcastExcept(const Message& msg, int exceptSocket);

		/**
		 * Send a message to a specific client.
		 */
		bool sendToClient(int socket, const Message& msg);

		/**
		 * Register a connected socket.
		 */
		void registerSocket(int socket, const std::string& playerName);

		/**
		 * Unregister a socket.
		 */
		void unregisterSocket(int socket);

		/**
		 * Get all connected sockets.
		 */
		[[nodiscard]] std::vector<int> getAllSockets() const;

		/**
		 * Timeout checker thread.
		 */
		void timeoutLoop(std::stop_token stopToken);

		/**
		 * Broadcast lobby state to all clients.
		 */
		void broadcastLobbyState();

		/**
		 * Broadcast game state to all clients.
		 */
		void broadcastGameState();
	};

} // namespace df::bifrost

