/**
 * @file midgard.cpp
 * @brief Implementation of client-side network handler
 */

#include "midgard.h"

#include <fmt/core.h>


namespace df::bifrost {

	// ═══════════════════════════════════════════════════════════════════════════════
	// CONSTRUCTOR / DESTRUCTOR
	// ═══════════════════════════════════════════════════════════════════════════════

	Midgard::Midgard() = default;

	Midgard::~Midgard() {
		disconnect();
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// CONNECTION
	// ═══════════════════════════════════════════════════════════════════════════════

	bool Midgard::connect(const std::string& host, uint16_t port) {
		std::lock_guard<std::mutex> lock(mutex_);

		if (connected_) {
			fmt::println("[Midgard] Already connected");
			return false;
		}

		try {
			tcpClient_.tryConnect(host, port);
			connected_ = true;
			shouldStop_ = false;

			// Start receive thread
			receiveThread_ = std::make_unique<std::jthread>([this](std::stop_token st) {
				receiveLoop(st);
			});

			fmt::println("[Midgard] Connected to {}:{}", host, port);

			if (connectionCallback_) {
				connectionCallback_(true, "Connected");
			}

			return true;
		} catch (const std::exception& e) {
			fmt::println("[Midgard] Connection failed: {}", e.what());

			if (connectionCallback_) {
				connectionCallback_(false, e.what());
			}

			return false;
		}
	}


	void Midgard::disconnect() {
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (!connected_) {
				return;
			}

			shouldStop_ = true;
			connected_ = false;
		}

		// Stop receive thread
		if (receiveThread_) {
			receiveThread_->request_stop();
			receiveThread_->join();
			receiveThread_.reset();
		}

		// Disconnect TCP
		tcpClient_.disconnect();

		// Clear state
		{
			std::lock_guard<std::mutex> lock(mutex_);
			playerId_.reset();
			playerName_.clear();
			isHost_ = false;
			lastLobbyState_.reset();
			lastGameState_.reset();
		}

		fmt::println("[Midgard] Disconnected");

		if (connectionCallback_) {
			connectionCallback_(false, "Disconnected");
		}
	}


	bool Midgard::isConnected() const {
		return connected_;
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// CALLBACKS
	// ═══════════════════════════════════════════════════════════════════════════════

	void Midgard::setLobbyStateCallback(LobbyStateCallback callback) {
		std::lock_guard<std::mutex> lock(mutex_);
		lobbyStateCallback_ = std::move(callback);
	}

	void Midgard::setGameStateCallback(GameStateCallback callback) {
		std::lock_guard<std::mutex> lock(mutex_);
		gameStateCallback_ = std::move(callback);
	}

	void Midgard::setActionResultCallback(ActionResultCallback callback) {
		std::lock_guard<std::mutex> lock(mutex_);
		actionResultCallback_ = std::move(callback);
	}

	void Midgard::setGameEventCallback(GameEventCallback callback) {
		std::lock_guard<std::mutex> lock(mutex_);
		gameEventCallback_ = std::move(callback);
	}

	void Midgard::setConnectionCallback(ConnectionCallback callback) {
		std::lock_guard<std::mutex> lock(mutex_);
		connectionCallback_ = std::move(callback);
	}

	void Midgard::setKickedCallback(KickedCallback callback) {
		std::lock_guard<std::mutex> lock(mutex_);
		kickedCallback_ = std::move(callback);
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// LOBBY ACTIONS
	// ═══════════════════════════════════════════════════════════════════════════════

	bool Midgard::join(const std::string& playerName) {
		if (!connected_) {
			fmt::println("[Midgard] Not connected, cannot join");
			return false;
		}

		{
			std::lock_guard<std::mutex> lock(mutex_);
			playerName_ = playerName;
		}

		Message msg;
		msg.type = MessageType::JOIN_REQUEST;
		msg.seq = getNextSeq();
		msg.payload = JoinRequestPayload{playerName};

		return sendMessage(msg);
	}


	void Midgard::leave() {
		if (!connected_) {
			return;
		}

		Message msg;
		msg.type = MessageType::LEAVE_REQUEST;
		msg.seq = getNextSeq();
		msg.payload = LeaveRequestPayload{};

		sendMessage(msg);
		disconnect();
	}


	void Midgard::setReady(bool ready) {
		if (!connected_) {
			return;
		}

		Message msg;
		msg.type = MessageType::READY_TOGGLE;
		msg.seq = getNextSeq();
		msg.payload = ReadyTogglePayload{ready};

		sendMessage(msg);
	}


	void Midgard::updateConfig(const LobbyConfig& config) {
		if (!connected_) {
			return;
		}

		Message msg;
		msg.type = MessageType::UPDATE_CONFIG;
		msg.seq = getNextSeq();
		msg.payload = UpdateConfigPayload{config};

		sendMessage(msg);
	}


	void Midgard::startGame() {
		if (!connected_) {
			return;
		}

		Message msg;
		msg.type = MessageType::START_GAME;
		msg.seq = getNextSeq();
		msg.payload = StartGamePayload{};

		sendMessage(msg);
	}


	void Midgard::kickPlayer(const std::string& playerName) {
		if (!connected_) {
			return;
		}

		Message msg;
		msg.type = MessageType::KICK_PLAYER;
		msg.seq = getNextSeq();
		msg.payload = KickPlayerPayload{playerName};

		sendMessage(msg);
	}


	void Midgard::reconnect(const std::string& playerName) {
		if (!connected_) {
			fmt::println("[Midgard] Not connected, cannot reconnect");
			return;
		}

		{
			std::lock_guard<std::mutex> lock(mutex_);
			playerName_ = playerName;
		}

		Message msg;
		msg.type = MessageType::RECONNECT;
		msg.seq = getNextSeq();
		msg.payload = ReconnectPayload{playerName};

		sendMessage(msg);
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// GAME ACTIONS
	// ═══════════════════════════════════════════════════════════════════════════════

	void Midgard::endTurn() {
		if (!connected_) {
			return;
		}

		Message msg;
		msg.type = MessageType::END_TURN;
		msg.seq = getNextSeq();
		msg.payload = EndTurnPayload{};

		sendMessage(msg);
	}


	void Midgard::buildSettlement(size_t vertexId) {
		if (!connected_) {
			return;
		}

		Message msg;
		msg.type = MessageType::BUILD_SETTLEMENT;
		msg.seq = getNextSeq();
		msg.payload = BuildSettlementPayload{vertexId};

		sendMessage(msg);
	}


	void Midgard::buildRoad(size_t edgeId, RoadLevel level) {
		if (!connected_) {
			return;
		}

		Message msg;
		msg.type = MessageType::BUILD_ROAD;
		msg.seq = getNextSeq();
		msg.payload = BuildRoadPayload{edgeId, level};

		sendMessage(msg);
	}


	void Midgard::moveHero(size_t targetTileId) {
		if (!connected_) {
			return;
		}

		Message msg;
		msg.type = MessageType::MOVE_HERO;
		msg.seq = getNextSeq();
		msg.payload = MoveHeroPayload{targetTileId};

		sendMessage(msg);
	}


	void Midgard::ping() {
		if (!connected_) {
			return;
		}

		int64_t timestamp = currentTimestampMs();
		lastPingTimestamp_ = timestamp;

		Message msg;
		msg.type = MessageType::PING;
		msg.seq = getNextSeq();
		msg.payload = PingPayload{timestamp};

		sendMessage(msg);
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// STATE QUERIES
	// ═══════════════════════════════════════════════════════════════════════════════

	std::optional<size_t> Midgard::getPlayerId() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return playerId_;
	}


	const std::string& Midgard::getPlayerName() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return playerName_;
	}


	std::optional<LobbyState> Midgard::getLastLobbyState() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return lastLobbyState_;
	}


	std::optional<nlohmann::json> Midgard::getLastGameState() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return lastGameState_;
	}


	bool Midgard::isHost() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return isHost_;
	}


	int64_t Midgard::getLatencyMs() const {
		return lastLatencyMs_;
	}


	// ═══════════════════════════════════════════════════════════════════════════════
	// PRIVATE METHODS
	// ═══════════════════════════════════════════════════════════════════════════════

	bool Midgard::sendMessage(const Message& msg) {
		if (!connected_) {
			return false;
		}

		try {
			// Get the socket from TcpClient - we need to access it for framing
			// Since TcpClient doesn't expose the socket, we'll use its send method
			// and do the framing ourselves
			std::vector<uint8_t> framedData = frameMessage(msg);
			tcpClient_.trySend(framedData);
			return true;
		} catch (const std::exception& e) {
			fmt::println("[Midgard] Send error: {}", e.what());
			handleDisconnect(e.what());
			return false;
		}
	}


	uint32_t Midgard::getNextSeq() {
		return nextSeq_++;
	}


	void Midgard::receiveLoop(std::stop_token stopToken) {
		fmt::println("[Midgard] Receive loop started");

		// We need direct socket access for readFramedMessage
		// Since TcpClient doesn't expose it, we'll use a buffer-based approach
		std::vector<uint8_t> buffer;
		buffer.reserve(4096);

		while (!stopToken.stop_requested() && connected_) {
			try {
				// Receive data using TcpClient's binary receive
				auto data = tcpClient_.tryReceiveBinary(4096);

				if (data.empty()) {
					if (!stopToken.stop_requested()) {
						handleDisconnect("Connection closed by server");
					}
					break;
				}

				// Append to buffer
				buffer.insert(buffer.end(), data.begin(), data.end());

				// Process complete messages from buffer
				while (buffer.size() >= HEADER_SIZE) {
					// Read length from header (big-endian)
					uint32_t length = (static_cast<uint32_t>(buffer[0]) << 24) |
									  (static_cast<uint32_t>(buffer[1]) << 16) |
									  (static_cast<uint32_t>(buffer[2]) << 8) |
									  static_cast<uint32_t>(buffer[3]);

					if (length > MAX_MESSAGE_SIZE) {
						fmt::println("[Midgard] Message too large: {} bytes", length);
						handleDisconnect("Protocol error: message too large");
						return;
					}

					// Check if we have the complete message
					if (buffer.size() < HEADER_SIZE + length) {
						break; // Wait for more data
					}

					// Extract and parse the message
					std::string jsonStr(buffer.begin() + HEADER_SIZE,
										buffer.begin() + HEADER_SIZE + length);

					try {
						nlohmann::json j = nlohmann::json::parse(jsonStr);
						Message msg = Message::deserialize(j);
						handleMessage(msg);
					} catch (const nlohmann::json::exception& e) {
						fmt::println("[Midgard] JSON parse error: {}", e.what());
					}

					// Remove processed data from buffer
					buffer.erase(buffer.begin(), buffer.begin() + HEADER_SIZE + length);
				}

			} catch (const std::exception& e) {
				if (!stopToken.stop_requested() && connected_) {
					fmt::println("[Midgard] Receive error: {}", e.what());
					handleDisconnect(e.what());
				}
				break;
			}
		}

		fmt::println("[Midgard] Receive loop ended");
	}


	void Midgard::handleMessage(const Message& msg) {
		fmt::println("[Midgard] Received message type: {}", messageTypeToString(msg.type));

		switch (msg.type) {
		case MessageType::JOIN_RESPONSE: {
			const auto& payload = std::get<JoinResponsePayload>(msg.payload);
			if (payload.success) {
				std::lock_guard<std::mutex> lock(mutex_);
				playerId_ = payload.playerId;
				fmt::println("[Midgard] Joined successfully with player ID {}", payload.playerId);
			} else {
				fmt::println("[Midgard] Join failed: {}",
							 payload.error ? payload.error->message : "Unknown error");
			}

			if (actionResultCallback_) {
				actionResultCallback_(msg.seq, payload.success, payload.error);
			}
			break;
		}

		case MessageType::LOBBY_STATE: {
			const auto& payload = std::get<LobbyStatePayload>(msg.payload);
			{
				std::lock_guard<std::mutex> lock(mutex_);
				lastLobbyState_ = payload.lobby;

				// Update host status
				if (playerId_) {
					for (const auto& p : payload.lobby.players) {
						if (p.playerId == *playerId_) {
							isHost_ = p.isHost;
							break;
						}
					}
				}
			}

			if (lobbyStateCallback_) {
				lobbyStateCallback_(payload.lobby);
			}
			break;
		}

		case MessageType::CONFIG_UPDATE: {
			const auto& payload = std::get<ConfigUpdatePayload>(msg.payload);
			{
				std::lock_guard<std::mutex> lock(mutex_);
				if (lastLobbyState_) {
					lastLobbyState_->config = payload.config;
				}
			}

			if (lobbyStateCallback_ && lastLobbyState_) {
				lobbyStateCallback_(*lastLobbyState_);
			}
			break;
		}

		case MessageType::KICKED: {
			const auto& payload = std::get<KickedPayload>(msg.payload);
			fmt::println("[Midgard] Kicked: {}", payload.reason);

			if (kickedCallback_) {
				kickedCallback_(payload.reason);
			}

			disconnect();
			break;
		}

		case MessageType::GAME_STARTED: {
			const auto& payload = std::get<GameStartedPayload>(msg.payload);
			{
				std::lock_guard<std::mutex> lock(mutex_);
				lastGameState_ = payload.initialState;
			}

			if (gameStateCallback_) {
				gameStateCallback_(payload.initialState);
			}

			if (gameEventCallback_) {
				gameEventCallback_(msg);
			}
			break;
		}

		case MessageType::GAME_STATE: {
			const auto& payload = std::get<GameStatePayload>(msg.payload);
			{
				std::lock_guard<std::mutex> lock(mutex_);
				lastGameState_ = payload.state;
			}

			if (gameStateCallback_) {
				gameStateCallback_(payload.state);
			}
			break;
		}

		case MessageType::ACTION_RESULT: {
			const auto& payload = std::get<ActionResultPayload>(msg.payload);

			if (actionResultCallback_) {
				actionResultCallback_(payload.seq, payload.success, payload.error);
			}
			break;
		}

		case MessageType::GAME_PAUSED:
		case MessageType::GAME_RESUMED:
		case MessageType::GAME_OVER: {
			if (gameEventCallback_) {
				gameEventCallback_(msg);
			}
			break;
		}

		case MessageType::PONG: {
			const auto& payload = std::get<PongPayload>(msg.payload);
			int64_t now = currentTimestampMs();
			lastLatencyMs_ = now - payload.timestamp;
			fmt::println("[Midgard] Ping latency: {}ms", lastLatencyMs_.load());
			break;
		}

		case MessageType::RECONNECT_RESPONSE: {
			const auto& payload = std::get<ReconnectResponsePayload>(msg.payload);
			if (payload.success) {
				std::lock_guard<std::mutex> lock(mutex_);
				playerId_ = payload.playerId;
				if (payload.state) {
					lastGameState_ = *payload.state;
				}
				fmt::println("[Midgard] Reconnected successfully with player ID {}", payload.playerId);

				if (payload.state && gameStateCallback_) {
					gameStateCallback_(*payload.state);
				}
			} else {
				fmt::println("[Midgard] Reconnect failed: {}",
							 payload.error ? payload.error->message : "Unknown error");
			}

			if (actionResultCallback_) {
				actionResultCallback_(msg.seq, payload.success, payload.error);
			}
			break;
		}

		case MessageType::SERVER_ERROR: {
			const auto& payload = std::get<ServerErrorPayload>(msg.payload);
			fmt::println("[Midgard] Server error: {} - {}",
						 errorCodeToString(payload.code), payload.message);

			if (actionResultCallback_) {
				actionResultCallback_(msg.seq, false, ErrorInfo{payload.code, payload.message});
			}
			break;
		}

		default:
			fmt::println("[Midgard] Unhandled message type: {}", messageTypeToString(msg.type));
			break;
		}
	}


	void Midgard::handleDisconnect(const std::string& reason) {
		bool wasConnected = connected_.exchange(false);

		if (wasConnected) {
			fmt::println("[Midgard] Connection lost: {}", reason);

			if (connectionCallback_) {
				connectionCallback_(false, reason);
			}
		}
	}


} // namespace df::bifrost

