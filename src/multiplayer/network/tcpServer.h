#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "multiplayer/network/socketPlatform.h"
namespace df::mp {
	class TcpServer {
	  public:
		// handler receives socket and stop_token for cooperative cancellation
		using ClientHandler = std::function<void(net::SocketHandle, std::stop_token)>;

		// use singleton pattern -> there should only be one server at a time
		static TcpServer& instance();

		void configure(uint16_t port, const std::string& bindTo = "0.0.0.0");
		void setMaxConnections(size_t maxConnections);
		void setRateLimit(size_t maxConnectionsPerWindow, std::chrono::seconds windowDuration);
		void start();
		void run();
		void stop();

		// called on a separate thread fro each client.
		// This shoudl be used to implement the protocol used by the game.
		// After it finishes, the socket is closed.
		void onClientCallback(ClientHandler handler);

		// disable copying/moving
		TcpServer(const TcpServer&) = delete;
		TcpServer& operator=(const TcpServer&) = delete;


	  private:
		TcpServer();
		~TcpServer();

		void handleClient(net::SocketHandle clientSocket, std::stop_token stopToken);
		void cleanupFinishedConnections();
		void cleanupStaleRateLimitEntries();
		size_t getActiveConnectionCount();
		bool isRateLimited(uint32_t clientIp);

		struct ClientConnection {
			std::jthread thread;
			std::atomic<bool> finished{false};
		};

		uint16_t port{0};
		net::SocketPlatform socketPlatform;
		net::SocketHandle serverSocket{net::INVALID_SOCKET_HANDLE};
		net::SocketAddress serverAddress{};

		ClientHandler clientHandler;

		std::atomic<bool> isRunning{false};

		// connection management
		std::mutex connectionsMutex;
		std::vector<std::unique_ptr<ClientConnection>> connections;
		size_t maxConnections{10};

		// rate limiting: track connection timestamps per IP
		std::mutex rateLimitMutex;
		std::unordered_map<uint32_t, std::deque<std::chrono::steady_clock::time_point>> connectionAttempts;
		size_t rateLimitMaxConnections{5};
		std::chrono::seconds rateLimitWindow{60};
	};
} // namespace df::mp
