/**
 * @file asgard.cpp
 * @brief Implementation of the Bifrost game server
 *
 * Created with AI-assistance
 */

#include "asgard.h"

#include <fmt/base.h>


namespace df::bifrost {

	// ═══════════════════════════════════════════════════════════════════════════════
	// CONSTRUCTOR / DESTRUCTOR
	// ═══════════════════════════════════════════════════════════════════════════════

	Asgard::Asgard() = default;

	Asgard::~Asgard() {
		stop();
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// CONFIGURATION
	// ═══════════════════════════════════════════════════════════════════════════════

	void Asgard::configure(uint16_t port, const std::string& bindAddress) {
		std::lock_guard<std::mutex> lock(mutex_);

		if (running_) {
			fmt::println("[Asgard] Cannot configure while running");
			return;
		}

		port_ = port;
		bindAddress_ = bindAddress;
	}


	void Asgard::setMaxConnections(size_t maxConnections) {
		std::lock_guard<std::mutex> lock(mutex_);

		if (running_) {
			fmt::println("[Asgard] Cannot configure while running");
			return;
		}

		maxConnections_ = maxConnections;
	}


	void Asgard::setBuildingCosts(const BuildingCosts& costs) {
		session_.setBuildingCosts(costs);
	}


	void Asgard::setInitialConfig(const LobbyConfig& config) {
		(void)config;
		// Update config through session's internal mechanism
		// This is a bit of a workaround since we're setting initial config
		// before any client connects
		// The first client (host) will be able to update it
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// SERVER LIFECYCLE
	// ═══════════════════════════════════════════════════════════════════════════════

	void Asgard::start() {
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (running_) {
				fmt::println("[Asgard] Server already running");
				return;
			}

			running_ = true;
		}

		// Configure and start TCP server
		auto& server = mp::TcpServer::instance();
		server.configure(port_, bindAddress_);
		server.setMaxConnections(maxConnections_);
		server.onClientCallback([this](mp::net::SocketHandle socket, std::stop_token stopToken) {
			handleClient(socket, stopToken);
		});

		// Start in background thread
		serverThread_ = std::make_unique<std::jthread>([this](std::stop_token) {
			auto& server = mp::TcpServer::instance();
			server.start();
			fmt::println("[Asgard] Server started on port {}", port_);
			server.run();
		});

		// Start timeout checker thread
		timeoutThread_ = std::make_unique<std::jthread>([this](std::stop_token st) {
			timeoutLoop(st);
		});

		fmt::println("[Asgard] Server starting...");
	}


	void Asgard::run() {
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (running_) {
				fmt::println("[Asgard] Server already running");
				return;
			}

			running_ = true;
		}

		// Configure and start TCP server
		auto& server = mp::TcpServer::instance();
		server.configure(port_, bindAddress_);
		server.setMaxConnections(maxConnections_);
		server.onClientCallback([this](mp::net::SocketHandle socket, std::stop_token stopToken) {
			handleClient(socket, stopToken);
		});

		// Start timeout checker in background
		timeoutThread_ = std::make_unique<std::jthread>([this](std::stop_token st) {
			timeoutLoop(st);
		});

		// Run server (blocking)
		server.start();
		fmt::println("[Asgard] Server started on port {}", port_);
		server.run();

		// When we return, server was stopped
		running_ = false;
	}


	void Asgard::stop() {
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (!running_) {
				return;
			}

			running_ = false;
		}

		// Stop the TCP server
		mp::TcpServer::instance().stop();

		// Stop threads
		if (timeoutThread_) {
			timeoutThread_->request_stop();
			timeoutThread_->join();
			timeoutThread_.reset();
		}

		if (serverThread_) {
			serverThread_->request_stop();
			serverThread_->join();
			serverThread_.reset();
		}

		// Clear sockets
		{
			std::lock_guard<std::mutex> lock(socketsMutex_);
			connectedSockets_.clear();
		}

		// Reset session
		session_.reset();

		fmt::println("[Asgard] Server stopped");
	}


	bool Asgard::isRunning() const {
		return running_;
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// STATE ACCESS
	// ═══════════════════════════════════════════════════════════════════════════════

	SessionManager& Asgard::getSession() {
		return session_;
	}


	const SessionManager& Asgard::getSession() const {
		return session_;
	}


	LobbyState Asgard::getLobbyState() const {
		return session_.getLobbyState();
	}


	size_t Asgard::getConnectedPlayerCount() const {
		return session_.getConnectedPlayerCount();
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// CLIENT HANDLING
	// ═══════════════════════════════════════════════════════════════════════════════

	void Asgard::handleClient(int socket, std::stop_token stopToken) {
		fmt::println("[Asgard] Client connected, socket {}", socket);

		std::string clientPlayerName;
		bool registered = false;

		while (!stopToken.stop_requested() && running_) {
			// Read message with timeout
			auto msgOpt = readFramedMessage(socket);

			if (!msgOpt) {
				// Connection closed or error
				break;
			}

			Message& msg = *msgOpt;
			fmt::println("[Asgard] Received {} from socket {}", messageTypeToString(msg.type), socket);

			// Handle JoinRequest specially to register the socket
			if (msg.type == MessageType::JOIN_REQUEST) {
				const auto& payload = std::get<JoinRequestPayload>(msg.payload);
				auto playerIdOpt = session_.addClient(socket, payload.playerName);

				if (playerIdOpt) {
					clientPlayerName = payload.playerName;
					registerSocket(socket, clientPlayerName);
					registered = true;

					// Send success response
					auto response = createJoinResponse(msg.seq, true, *playerIdOpt);
					sendToClient(socket, response);

					// Broadcast lobby state to all
					broadcastLobbyState();
				} else {
					// Determine error reason
					ErrorCode code = ErrorCode::INTERNAL_ERROR;
					std::string reason = "Failed to join";

					if (session_.getState() != SessionState::LOBBY) {
						code = ErrorCode::GAME_ALREADY_STARTED;
						reason = "Game already in progress";
					} else if (session_.getConnectedPlayerCount() >= MAX_PLAYERS) {
						code = ErrorCode::LOBBY_FULL;
						reason = "Lobby is full";
					} else {
						code = ErrorCode::NAME_TAKEN;
						reason = "Player name already taken";
					}

					auto response = createJoinResponse(msg.seq, false, 0,
													   ErrorInfo{code, reason});
					sendToClient(socket, response);
				}
				continue;
			}

			// Handle Reconnect specially
			if (msg.type == MessageType::RECONNECT) {
				const auto& payload = std::get<ReconnectPayload>(msg.payload);
				auto playerIdOpt = session_.handleReconnect(socket, payload.playerName);

				Message response;
				response.type = MessageType::RECONNECT_RESPONSE;
				response.seq = msg.seq;

				if (playerIdOpt) {
					clientPlayerName = payload.playerName;
					registerSocket(socket, clientPlayerName);
					registered = true;

					ReconnectResponsePayload respPayload;
					respPayload.success = true;
					respPayload.playerId = *playerIdOpt;

					if (session_.getState() == SessionState::PLAYING ||
						session_.getState() == SessionState::PAUSED) {
						respPayload.state = session_.getSerializedGameState();
					}

					response.payload = respPayload;
					sendToClient(socket, response);

					// Broadcast game resumed if applicable
					if (session_.getState() == SessionState::PLAYING) {
						Message resumed;
						resumed.type = MessageType::GAME_RESUMED;
						resumed.seq = 0;
						resumed.payload = GameResumedPayload{clientPlayerName};
						broadcast(resumed);
					}
				} else {
					ReconnectResponsePayload respPayload;
					respPayload.success = false;
					respPayload.error = ErrorInfo{ErrorCode::PLAYER_NOT_FOUND,
												  "No disconnected player with that name"};
					response.payload = respPayload;
					sendToClient(socket, response);
				}
				continue;
			}

			// Process other messages through session manager
			auto result = session_.processMessage(socket, msg);

			// Send response to client
			sendToClient(socket, result.response);

			// Handle broadcasts
			if (result.broadcastLobby) {
				broadcastLobbyState();
			}

			if (result.broadcastGameState) {
				broadcastGameState();
			}

			// Handle GameStarted specially - send to all clients
			if (msg.type == MessageType::START_GAME && result.response.type == MessageType::GAME_STARTED) {
				// The response already contains the initial state
				// Broadcast it to all other clients
				broadcastExcept(result.response, socket);
			}

			if (result.additionalBroadcast) {
				broadcast(*result.additionalBroadcast);
			}
		}

		// Client disconnected
		fmt::println("[Asgard] Client disconnected, socket {}", socket);

		if (registered) {
			unregisterSocket(socket);

			// Mark as disconnected in session (for potential reconnect)
			session_.markClientDisconnected(socket);

			// If game was playing, broadcast pause
			if (session_.getState() == SessionState::PAUSED) {
				Message paused;
				paused.type = MessageType::GAME_PAUSED;
				paused.seq = 0;
				paused.payload = GamePausedPayload{
					clientPlayerName,
					session_.getConfig().reconnectTimeoutSeconds};
				broadcast(paused);
			} else if (session_.getState() == SessionState::LOBBY) {
				// In lobby, just broadcast updated state
				broadcastLobbyState();
			}
		}
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// BROADCASTING
	// ═══════════════════════════════════════════════════════════════════════════════

	void Asgard::broadcast(const Message& msg) {
		auto sockets = getAllSockets();
		for (int socket : sockets) {
			sendToClient(socket, msg);
		}
	}


	void Asgard::broadcastExcept(const Message& msg, int exceptSocket) {
		auto sockets = getAllSockets();
		for (int socket : sockets) {
			if (socket != exceptSocket) {
				sendToClient(socket, msg);
			}
		}
	}


	bool Asgard::sendToClient(int socket, const Message& msg) {
		bool success = sendFramedMessage(socket, msg);
		if (!success) {
			fmt::println("[Asgard] Failed to send to socket {}", socket);
		}
		return success;
	}


	void Asgard::registerSocket(int socket, const std::string& playerName) {
		std::lock_guard<std::mutex> lock(socketsMutex_);
		connectedSockets_[socket] = playerName;
	}


	void Asgard::unregisterSocket(int socket) {
		std::lock_guard<std::mutex> lock(socketsMutex_);
		connectedSockets_.erase(socket);
	}


	std::vector<int> Asgard::getAllSockets() const {
		std::lock_guard<std::mutex> lock(socketsMutex_);
		std::vector<int> sockets;
		sockets.reserve(connectedSockets_.size());
		for (const auto& [socket, name] : connectedSockets_) {
			sockets.push_back(socket);
		}
		return sockets;
	}


	void Asgard::broadcastLobbyState() {
		LobbyState lobby = session_.getLobbyState();
		Message msg = createLobbyStateMessage(0, lobby);
		broadcast(msg);
	}


	void Asgard::broadcastGameState() {
		nlohmann::json state = session_.getSerializedGameState();
		Message msg = createGameStateMessage(0, state);
		broadcast(msg);
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// TIMEOUT HANDLING
	// ═══════════════════════════════════════════════════════════════════════════════

	void Asgard::timeoutLoop(std::stop_token stopToken) {
		fmt::println("[Asgard] Timeout checker started");

		while (!stopToken.stop_requested() && running_) {
			// Check every second
			std::this_thread::sleep_for(std::chrono::seconds(1));

			if (!running_ || stopToken.stop_requested()) {
				break;
			}

			// Check for reconnect timeouts
			auto kickedPlayers = session_.checkReconnectTimeouts();

			for (const auto& playerName : kickedPlayers) {
				fmt::println("[Asgard] Player '{}' timed out, removed from game", playerName);

				// Notify remaining clients
				if (session_.getState() == SessionState::PLAYING) {
					// If game resumed after kicking timed-out player
					Message resumed;
					resumed.type = MessageType::GAME_RESUMED;
					resumed.seq = 0;
					resumed.payload = GameResumedPayload{playerName + " (timed out)"};
					broadcast(resumed);

					// Broadcast updated state
					broadcastGameState();
				}
			}
		}

		fmt::println("[Asgard] Timeout checker stopped");
	}


} // namespace df::bifrost
