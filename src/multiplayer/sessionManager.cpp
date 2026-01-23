/**
 * @file sessionManager.cpp
 * @brief Implementation of server-side game session management
 */

#include "sessionManager.h"

#include <algorithm>
#include <fmt/core.h>


namespace df::bifrost {

// ═══════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR / DESTRUCTOR
// ═══════════════════════════════════════════════════════════════════════════════

SessionManager::SessionManager()
	: rng_(std::random_device{}()) {
	reset();
}

SessionManager::~SessionManager() = default;


// ═══════════════════════════════════════════════════════════════════════════════
// SESSION LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════════════

void SessionManager::reset() {
	std::lock_guard<std::mutex> lock(mutex_);

	state_ = SessionState::LOBBY;
	clients_.clear();
	nextPlayerId_ = 0;
	hostSocket_.reset();
	config_ = LobbyConfig{};
	buildingCosts_ = BuildingCosts{};

	// Reset game objects
	registry_ = std::make_unique<Registry>();
	gameState_ = std::make_unique<GameState>(registry_.get());
	gameController_.reset();

	// Re-seed RNG
	rng_.seed(std::random_device{}());

	fmt::println("[SessionManager] Session reset to LOBBY state");
}


SessionState SessionManager::getState() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return state_;
}


bool SessionManager::canStartGame() const {
	std::lock_guard<std::mutex> lock(mutex_);

	if (state_ != SessionState::LOBBY) {
		return false;
	}

	size_t connectedCount = 0;
	bool allReady = true;

	for (const auto& [socket, client] : clients_) {
		if (client.connected) {
			connectedCount++;
			if (!client.ready) {
				allReady = false;
			}
		}
	}

	return connectedCount >= MIN_PLAYERS && connectedCount <= MAX_PLAYERS && allReady;
}


// ═══════════════════════════════════════════════════════════════════════════════
// CLIENT MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════════

std::optional<size_t> SessionManager::addClient(int socket, const std::string& playerName) {
	std::lock_guard<std::mutex> lock(mutex_);

	// Check if game already started
	if (state_ != SessionState::LOBBY) {
		fmt::println("[SessionManager] Rejecting join: game already started");
		return std::nullopt;
	}

	// Check player count
	if (clients_.size() >= MAX_PLAYERS) {
		fmt::println("[SessionManager] Rejecting join: lobby full");
		return std::nullopt;
	}

	// Check for duplicate name
	for (const auto& [s, client] : clients_) {
		if (client.playerName == playerName) {
			fmt::println("[SessionManager] Rejecting join: name '{}' already taken", playerName);
			return std::nullopt;
		}
	}

	// Assign player ID
	size_t playerId = nextPlayerId_++;
	bool isFirstPlayer = clients_.empty();

	ConnectedClient client;
	client.socket = socket;
	client.playerId = playerId;
	client.playerName = playerName;
	client.ready = false;
	client.isHost = isFirstPlayer;
	client.connected = true;

	clients_[socket] = client;

	if (isFirstPlayer) {
		hostSocket_ = socket;
	}

	fmt::println("[SessionManager] Player '{}' joined with ID {} (host: {})",
		playerName, playerId, isFirstPlayer);

	return playerId;
}


void SessionManager::removeClient(int socket) {
	std::lock_guard<std::mutex> lock(mutex_);

	auto it = clients_.find(socket);
	if (it == clients_.end()) {
		return;
	}

	std::string playerName = it->second.playerName;
	bool wasHost = it->second.isHost;
	clients_.erase(it);

	fmt::println("[SessionManager] Player '{}' removed", playerName);

	// If host left and we're in lobby, assign new host
	if (wasHost && state_ == SessionState::LOBBY && !clients_.empty()) {
		auto& newHost = clients_.begin()->second;
		newHost.isHost = true;
		hostSocket_ = clients_.begin()->first;
		fmt::println("[SessionManager] New host: '{}'", newHost.playerName);
	}

	// If in game and player left, we might need to handle this
	// For now, just mark as disconnected if game is running
	if (state_ == SessionState::PLAYING) {
		// Game continues - player is simply removed
		// In a production system, you might want to handle this differently
	}
}


void SessionManager::markClientDisconnected(int socket) {
	std::lock_guard<std::mutex> lock(mutex_);

	auto it = clients_.find(socket);
	if (it == clients_.end()) {
		return;
	}

	it->second.connected = false;
	it->second.disconnectTime = std::chrono::steady_clock::now();

	fmt::println("[SessionManager] Player '{}' disconnected", it->second.playerName);

	// If in game, transition to paused
	if (state_ == SessionState::PLAYING) {
		state_ = SessionState::PAUSED;
		fmt::println("[SessionManager] Game PAUSED due to disconnect");
	}
}


std::optional<size_t> SessionManager::handleReconnect(int socket, const std::string& playerName) {
	std::lock_guard<std::mutex> lock(mutex_);

	// Find disconnected client by name
	for (auto& [oldSocket, client] : clients_) {
		if (client.playerName == playerName && !client.connected) {
			// Update socket
			size_t playerId = client.playerId;
			bool wasHost = client.isHost;

			// Remove old entry, add new one with new socket
			ConnectedClient reconnectedClient = client;
			reconnectedClient.socket = socket;
			reconnectedClient.connected = true;

			clients_.erase(oldSocket);
			clients_[socket] = reconnectedClient;

			if (wasHost) {
				hostSocket_ = socket;
			}

			fmt::println("[SessionManager] Player '{}' reconnected", playerName);

			// Check if we can resume
			bool anyDisconnected = false;
			for (const auto& [s, c] : clients_) {
				if (!c.connected) {
					anyDisconnected = true;
					break;
				}
			}

			if (!anyDisconnected && state_ == SessionState::PAUSED) {
				state_ = SessionState::PLAYING;
				fmt::println("[SessionManager] Game RESUMED");
			}

			return playerId;
		}
	}

	fmt::println("[SessionManager] Reconnect failed: no disconnected player named '{}'", playerName);
	return std::nullopt;
}


ConnectedClient* SessionManager::getClientBySocket(int socket) {
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = clients_.find(socket);
	return (it != clients_.end()) ? &it->second : nullptr;
}


const ConnectedClient* SessionManager::getClientBySocket(int socket) const {
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = clients_.find(socket);
	return (it != clients_.end()) ? &it->second : nullptr;
}


ConnectedClient* SessionManager::getClientByName(const std::string& name) {
	std::lock_guard<std::mutex> lock(mutex_);
	for (auto& [socket, client] : clients_) {
		if (client.playerName == name) {
			return &client;
		}
	}
	return nullptr;
}


std::vector<int> SessionManager::getAllClientSockets() const {
	std::lock_guard<std::mutex> lock(mutex_);
	std::vector<int> sockets;
	sockets.reserve(clients_.size());
	for (const auto& [socket, client] : clients_) {
		if (client.connected) {
			sockets.push_back(socket);
		}
	}
	return sockets;
}


size_t SessionManager::getConnectedPlayerCount() const {
	std::lock_guard<std::mutex> lock(mutex_);
	size_t count = 0;
	for (const auto& [socket, client] : clients_) {
		if (client.connected) {
			count++;
		}
	}
	return count;
}


bool SessionManager::areAllPlayersReady() const {
	std::lock_guard<std::mutex> lock(mutex_);
	for (const auto& [socket, client] : clients_) {
		if (client.connected && !client.ready) {
			return false;
		}
	}
	return !clients_.empty();
}


// ═══════════════════════════════════════════════════════════════════════════════
// MESSAGE HANDLING
// ═══════════════════════════════════════════════════════════════════════════════

SessionManager::MessageResult SessionManager::processMessage(int socket, const Message& msg) {
	MessageResult result;
	result.response.seq = msg.seq;

	// Validate action is allowed in current state
	auto validationError = validateAction(socket, msg.type);
	if (validationError) {
		result.response = createActionResult(msg.seq, false, validationError);
		return result;
	}

	switch (msg.type) {
		case MessageType::READY_TOGGLE: {
			const auto& payload = std::get<ReadyTogglePayload>(msg.payload);
			setPlayerReady(socket, payload.ready);
			result.response = createActionResult(msg.seq, true);
			result.broadcastLobby = true;
			break;
		}

		case MessageType::UPDATE_CONFIG: {
			const auto& payload = std::get<UpdateConfigPayload>(msg.payload);
			bool success = updateConfig(socket, payload.config);
			if (success) {
				result.response = createActionResult(msg.seq, true);
				result.broadcastLobby = true;
			} else {
				result.response = createActionResult(msg.seq, false,
					ErrorInfo{ErrorCode::NOT_HOST, "Only host can update config"});
			}
			break;
		}

		case MessageType::START_GAME: {
			bool success = startGame(socket);
			if (success) {
				result.response.type = MessageType::GAME_STARTED;
				result.response.payload = GameStartedPayload{getSerializedGameState()};
				result.broadcastGameState = true;
			} else {
				std::string reason = !isHost(socket) ? "Only host can start game" :
					!canStartGame() ? "Not all players ready or not enough players" : "Unknown error";
				result.response = createActionResult(msg.seq, false,
					ErrorInfo{ErrorCode::NOT_ALL_READY, reason});
			}
			break;
		}

		case MessageType::KICK_PLAYER: {
			const auto& payload = std::get<KickPlayerPayload>(msg.payload);
			bool success = kickPlayer(socket, payload.playerName);
			result.response = createActionResult(msg.seq, success,
				success ? std::nullopt : std::optional<ErrorInfo>{
					ErrorInfo{ErrorCode::NOT_HOST, "Only host can kick players"}});
			result.broadcastLobby = success;
			break;
		}

		case MessageType::END_TURN: {
			bool success = endTurn(socket);
			result.response = createActionResult(msg.seq, success,
				success ? std::nullopt : std::optional<ErrorInfo>{
					ErrorInfo{ErrorCode::NOT_YOUR_TURN, "Not your turn"}});
			result.broadcastGameState = success;
			break;
		}

		case MessageType::BUILD_SETTLEMENT: {
			const auto& payload = std::get<BuildSettlementPayload>(msg.payload);
			auto [success, error] = buildSettlement(socket, payload.vertexId);
			result.response = createActionResult(msg.seq, success, error);
			result.broadcastGameState = success;
			break;
		}

		case MessageType::BUILD_ROAD: {
			const auto& payload = std::get<BuildRoadPayload>(msg.payload);
			auto [success, error] = buildRoad(socket, payload.edgeId, payload.level);
			result.response = createActionResult(msg.seq, success, error);
			result.broadcastGameState = success;
			break;
		}

		case MessageType::MOVE_HERO: {
			const auto& payload = std::get<MoveHeroPayload>(msg.payload);
			auto [success, error] = moveHero(socket, payload.targetTileId);
			result.response = createActionResult(msg.seq, success, error);
			result.broadcastGameState = success;
			break;
		}

		case MessageType::PING: {
			const auto& payload = std::get<PingPayload>(msg.payload);
			result.response = createPongMessage(msg.seq, payload.timestamp);
			break;
		}

		default:
			result.response = createErrorMessage(msg.seq, ErrorCode::INVALID_MESSAGE,
				"Unknown or unsupported message type");
			break;
	}

	return result;
}


// ═══════════════════════════════════════════════════════════════════════════════
// LOBBY OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════════

void SessionManager::setPlayerReady(int socket, bool ready) {
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = clients_.find(socket);
	if (it != clients_.end()) {
		it->second.ready = ready;
		fmt::println("[SessionManager] Player '{}' ready: {}", it->second.playerName, ready);
	}
}


bool SessionManager::updateConfig(int socket, const LobbyConfig& config) {
	std::lock_guard<std::mutex> lock(mutex_);

	if (!hostSocket_ || *hostSocket_ != socket) {
		return false;
	}

	config_ = config;
	fmt::println("[SessionManager] Config updated by host");
	return true;
}


bool SessionManager::kickPlayer(int socket, const std::string& playerName) {
	// Note: Don't hold lock while calling removeClient (which also locks)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (!hostSocket_ || *hostSocket_ != socket) {
			return false;
		}
	}

	// Find socket by name
	int targetSocket = -1;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		for (const auto& [s, client] : clients_) {
			if (client.playerName == playerName && s != socket) {
				targetSocket = s;
				break;
			}
		}
	}

	if (targetSocket == -1) {
		return false;
	}

	removeClient(targetSocket);
	return true;
}


bool SessionManager::startGame(int socket) {
	{
		std::lock_guard<std::mutex> lock(mutex_);

		if (!hostSocket_ || *hostSocket_ != socket) {
			return false;
		}

		if (state_ != SessionState::LOBBY) {
			return false;
		}
	}

	if (!canStartGame()) {
		return false;
	}

	// Initialize the game
	initializeGame();

	{
		std::lock_guard<std::mutex> lock(mutex_);
		state_ = SessionState::PLAYING;
	}

	fmt::println("[SessionManager] Game STARTED");
	return true;
}


LobbyState SessionManager::getLobbyState() const {
	std::lock_guard<std::mutex> lock(mutex_);

	LobbyState lobby;
	lobby.config = config_;
	lobby.hostId = hostSocket_ ? clients_.at(*hostSocket_).playerId : 0;
	lobby.minPlayers = MIN_PLAYERS;
	lobby.maxPlayers = MAX_PLAYERS;

	for (const auto& [socket, client] : clients_) {
		PlayerInfo info;
		info.playerId = client.playerId;
		info.name = client.playerName;
		info.ready = client.ready;
		info.isHost = client.isHost;
		info.connected = client.connected;
		lobby.players.push_back(info);
	}

	return lobby;
}


const LobbyConfig& SessionManager::getConfig() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return config_;
}


// ═══════════════════════════════════════════════════════════════════════════════
// GAME OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════════

bool SessionManager::endTurn(int socket) {
	std::lock_guard<std::mutex> lock(mutex_);

	if (state_ != SessionState::PLAYING) {
		return false;
	}

	if (!isPlayersTurn(socket)) {
		return false;
	}

	gameController_->endTurn();
	gameController_->startTurn();

	return true;
}


std::pair<bool, std::optional<ErrorInfo>> SessionManager::buildSettlement(int socket, size_t vertexId) {
	std::lock_guard<std::mutex> lock(mutex_);

	if (state_ != SessionState::PLAYING) {
		return {false, ErrorInfo{ErrorCode::INVALID_ACTION, "Game not in progress"}};
	}

	auto playerIdOpt = getPlayerIdBySocket(socket);
	if (!playerIdOpt) {
		return {false, ErrorInfo{ErrorCode::PLAYER_NOT_FOUND, "Player not found"}};
	}

	size_t playerId = *playerIdOpt;

	// Check turn
	if (gameState_->getCurrentPlayerId() != playerId) {
		return {false, ErrorInfo{ErrorCode::NOT_YOUR_TURN, "Not your turn"}};
	}

	// Check if can build
	if (!gameController_->canBuildSettlement(playerId, vertexId)) {
		return {false, ErrorInfo{ErrorCode::INVALID_LOCATION, "Cannot build settlement at this location"}};
	}

	// Get costs from server config
	auto costs = buildingCosts_.getSettlementCostVector();

	// Try to build
	bool success = gameController_->buildSettlement(playerId, vertexId, costs);

	if (!success) {
		return {false, ErrorInfo{ErrorCode::INSUFFICIENT_RESOURCES, "Not enough resources"}};
	}

	return {true, std::nullopt};
}


std::pair<bool, std::optional<ErrorInfo>> SessionManager::buildRoad(int socket, size_t edgeId, RoadLevel level) {
	std::lock_guard<std::mutex> lock(mutex_);

	if (state_ != SessionState::PLAYING) {
		return {false, ErrorInfo{ErrorCode::INVALID_ACTION, "Game not in progress"}};
	}

	auto playerIdOpt = getPlayerIdBySocket(socket);
	if (!playerIdOpt) {
		return {false, ErrorInfo{ErrorCode::PLAYER_NOT_FOUND, "Player not found"}};
	}

	size_t playerId = *playerIdOpt;

	// Check turn
	if (gameState_->getCurrentPlayerId() != playerId) {
		return {false, ErrorInfo{ErrorCode::NOT_YOUR_TURN, "Not your turn"}};
	}

	// Check if can build
	if (!gameController_->canBuildRoad(playerId, edgeId)) {
		return {false, ErrorInfo{ErrorCode::INVALID_LOCATION, "Cannot build road at this location"}};
	}

	// Get costs from server config
	auto costs = buildingCosts_.getRoadCostVector(level);

	// Try to build
	bool success = gameController_->buildRoad(playerId, edgeId, level, costs);

	if (!success) {
		return {false, ErrorInfo{ErrorCode::INSUFFICIENT_RESOURCES, "Not enough resources"}};
	}

	return {true, std::nullopt};
}


std::pair<bool, std::optional<ErrorInfo>> SessionManager::moveHero(int socket, size_t targetTileId) {
	std::lock_guard<std::mutex> lock(mutex_);

	if (state_ != SessionState::PLAYING) {
		return {false, ErrorInfo{ErrorCode::INVALID_ACTION, "Game not in progress"}};
	}

	auto playerIdOpt = getPlayerIdBySocket(socket);
	if (!playerIdOpt) {
		return {false, ErrorInfo{ErrorCode::PLAYER_NOT_FOUND, "Player not found"}};
	}

	size_t playerId = *playerIdOpt;

	// Check turn
	if (gameState_->getCurrentPlayerId() != playerId) {
		return {false, ErrorInfo{ErrorCode::NOT_YOUR_TURN, "Not your turn"}};
	}

	// TODO: Hero movement is a stub for now
	// bool success = gameController_->moveHeroToTile(playerId, targetTileId);
	(void)targetTileId;

	fmt::println("[SessionManager] Hero movement not fully implemented (stub)");
	return {false, ErrorInfo{ErrorCode::INVALID_ACTION, "Hero movement not fully implemented"}};
}


nlohmann::json SessionManager::getSerializedGameState() const {
	std::lock_guard<std::mutex> lock(mutex_);
	if (gameState_) {
		return gameState_->serialize();
	}
	return nlohmann::json::object();
}


bool SessionManager::isPlayersTurn(int socket) const {
	// Note: caller should hold mutex_

	auto playerIdOpt = getPlayerIdBySocket(socket);
	if (!playerIdOpt) {
		return false;
	}

	return gameState_ && gameState_->getCurrentPlayerId() == *playerIdOpt;
}


// ═══════════════════════════════════════════════════════════════════════════════
// PAUSE/RECONNECT
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<std::string> SessionManager::checkReconnectTimeouts() {
	std::lock_guard<std::mutex> lock(mutex_);

	std::vector<std::string> kickedPlayers;
	auto now = std::chrono::steady_clock::now();

	std::vector<int> socketsToRemove;

	for (const auto& [socket, client] : clients_) {
		if (!client.connected) {
			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
				now - client.disconnectTime).count();

			if (elapsed >= static_cast<long>(config_.reconnectTimeoutSeconds)) {
				kickedPlayers.push_back(client.playerName);
				socketsToRemove.push_back(socket);
			}
		}
	}

	// Remove timed-out clients
	for (int socket : socketsToRemove) {
		clients_.erase(socket);
	}

	// Check if we can resume
	if (!kickedPlayers.empty()) {
		bool anyDisconnected = false;
		for (const auto& [s, c] : clients_) {
			if (!c.connected) {
				anyDisconnected = true;
				break;
			}
		}

		if (!anyDisconnected && state_ == SessionState::PAUSED) {
			state_ = SessionState::PLAYING;
			fmt::println("[SessionManager] Game RESUMED after timeout kicks");
		}
	}

	return kickedPlayers;
}


bool SessionManager::hasDisconnectedPlayers() const {
	std::lock_guard<std::mutex> lock(mutex_);
	for (const auto& [socket, client] : clients_) {
		if (!client.connected) {
			return true;
		}
	}
	return false;
}


std::string SessionManager::getFirstDisconnectedPlayerName() const {
	std::lock_guard<std::mutex> lock(mutex_);
	for (const auto& [socket, client] : clients_) {
		if (!client.connected) {
			return client.playerName;
		}
	}
	return "";
}


// ═══════════════════════════════════════════════════════════════════════════════
// BUILDING COSTS
// ═══════════════════════════════════════════════════════════════════════════════

const BuildingCosts& SessionManager::getBuildingCosts() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return buildingCosts_;
}


void SessionManager::setBuildingCosts(const BuildingCosts& costs) {
	std::lock_guard<std::mutex> lock(mutex_);
	buildingCosts_ = costs;
}


// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE METHODS
// ═══════════════════════════════════════════════════════════════════════════════

std::optional<size_t> SessionManager::getPlayerIdBySocket(int socket) const {
	// Note: caller should hold mutex_ or this is called from a method that does
	auto it = clients_.find(socket);
	if (it != clients_.end()) {
		return it->second.playerId;
	}
	return std::nullopt;
}


bool SessionManager::isHost(int socket) const {
	// Note: caller should hold mutex_ or this is called from a method that does
	return hostSocket_ && *hostSocket_ == socket;
}


void SessionManager::initializeGame() {
	std::lock_guard<std::mutex> lock(mutex_);

	// Seed RNG from config (0 means random)
	uint32_t seed = config_.seed;
	if (seed == 0) {
		seed = std::random_device{}();
	}
	rng_.seed(seed);

	// Create fresh registry and game state
	registry_ = std::make_unique<Registry>();
	gameState_ = std::make_unique<GameState>(registry_.get());

	// Generate the world using Graph::regenerate
	WorldGeneratorConfig worldConfig = config_.toWorldGeneratorConfig();
	worldConfig.seed = seed;  // Use actual seed

	gameState_->getMap().regenerate(worldConfig);

	// Create players
	createPlayers();

	// Initialize game controller
	gameController_ = std::make_unique<GameController>(*gameState_);

	// Set initial game phase
	gameState_->setPhase(types::GamePhase::PLAY);
	gameState_->setCurrentPlayerId(0);
	gameState_->setTurnCount(0);
	gameState_->setRoundNumber(0);

	// Start first player's turn
	gameController_->startTurn();

	fmt::println("[SessionManager] Game initialized with seed {}, {} players",
		seed, clients_.size());
}


void SessionManager::createPlayers() {
	// Note: caller should hold mutex_

	gameState_->clearPlayers();

	// Create players in order of their IDs
	std::vector<std::pair<size_t, std::string>> playerList;
	for (const auto& [socket, client] : clients_) {
		if (client.connected) {
			playerList.emplace_back(client.playerId, client.playerName);
		}
	}

	// Sort by player ID
	std::sort(playerList.begin(), playerList.end(),
		[](const auto& a, const auto& b) { return a.first < b.first; });

	for (const auto& [playerId, name] : playerList) {
		Player player(playerId);
		// Initialize with some starting resources (can be configured)
		player.addResources(types::TileType::FOREST, 2);
		player.addResources(types::TileType::CLAY, 2);
		player.addResources(types::TileType::GRASS, 2);
		player.addResources(types::TileType::FIELD, 2);
		player.addResources(types::TileType::MOUNTAIN, 1);

		gameState_->addPlayer(player);

		fmt::println("[SessionManager] Created player {} '{}'", playerId, name);
	}
}


std::optional<ErrorInfo> SessionManager::validateAction(int socket, MessageType action) const {
	std::lock_guard<std::mutex> lock(mutex_);

	// Check if client exists
	if (clients_.find(socket) == clients_.end()) {
		return ErrorInfo{ErrorCode::PLAYER_NOT_FOUND, "Unknown client"};
	}

	// State-based validation
	switch (action) {
		// Lobby-only actions
		case MessageType::READY_TOGGLE:
		case MessageType::UPDATE_CONFIG:
		case MessageType::START_GAME:
		case MessageType::KICK_PLAYER:
			if (state_ != SessionState::LOBBY) {
				return ErrorInfo{ErrorCode::INVALID_ACTION, "Action only allowed in lobby"};
			}
			break;

		// Game-only actions
		case MessageType::END_TURN:
		case MessageType::BUILD_SETTLEMENT:
		case MessageType::BUILD_ROAD:
		case MessageType::MOVE_HERO:
			if (state_ == SessionState::LOBBY) {
				return ErrorInfo{ErrorCode::INVALID_ACTION, "Game not started"};
			}
			if (state_ == SessionState::PAUSED) {
				return ErrorInfo{ErrorCode::INVALID_ACTION, "Game is paused"};
			}
			if (state_ == SessionState::ENDED) {
				return ErrorInfo{ErrorCode::INVALID_ACTION, "Game has ended"};
			}
			break;

		// Always allowed
		case MessageType::PING:
		case MessageType::LEAVE_REQUEST:
			break;

		default:
			break;
	}

	return std::nullopt;
}


}  // namespace df::bifrost

