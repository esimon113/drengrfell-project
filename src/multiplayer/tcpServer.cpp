#include "tcpServer.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fmt/base.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <stdexcept>
#include <stop_token>
#include <sys/socket.h>
#include <unistd.h>



namespace df::mp {
	TcpServer& TcpServer::instance() {
		static TcpServer srv;
		return srv;
	}


	TcpServer::TcpServer() = default;


	TcpServer::~TcpServer() {
		this->stop();
	}


	void TcpServer::configure(uint16_t port, const std::string& bindTo) {
		if (this->isRunning.load()) {
			throw std::runtime_error("[TcpServer] Cannot configure the server while running!");
		}
		this->port = port;

		this->serverAddress = {};
		this->serverAddress.sin_family = AF_INET;
		this->serverAddress.sin_port = htons(this->port);

		if (inet_pton(AF_INET, bindTo.c_str(), &this->serverAddress.sin_addr) <= 0) {
			throw std::runtime_error("[TcpServer] Invalid bind address");
		}
	}


	void TcpServer::setMaxConnections(size_t maxConnections) {
		if (this->isRunning.load()) {
			throw std::runtime_error("[TcpServer] Cannot configure while running!");
		}
		this->maxConnections = maxConnections;
	}


	void TcpServer::setRateLimit(size_t maxConnectionsPerWindow, std::chrono::seconds windowDuration) {
		if (this->isRunning.load()) {
			throw std::runtime_error("[TcpServer] Cannot configure while running!");
		}
		this->rateLimitMaxConnections = maxConnectionsPerWindow;
		this->rateLimitWindow = windowDuration;
	}


	void TcpServer::onClientCallback(ClientHandler handler) {
		if (this->isRunning.load()) {
			throw std::runtime_error("[TcpServer] Cannot change callback while running!");
		}
		this->clientHandler = std::move(handler);
	}


	void TcpServer::start() {
		if (this->isRunning.load()) {
			return;
		}

		if (this->port == 0) {
			throw std::runtime_error("[TcpServer] Port is not configured!");
		}

		this->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (this->serverSocket < 0) {
			throw std::runtime_error("[TcpServer] Creating a socket failed!");
		}

		int opt = 1;
		if (setsockopt(this->serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
			close(this->serverSocket);
			this->serverSocket = -1;
			throw std::runtime_error("[TcpServer] setsockopt(SO_REUSEADDR) failed");
		}

		if (bind(this->serverSocket, reinterpret_cast<sockaddr*>(&this->serverAddress), sizeof(this->serverAddress)) < 0) {
			close(this->serverSocket);
			this->serverSocket = -1;
			throw std::runtime_error("[TcpServer] bind() failed");
		}

		if (listen(this->serverSocket, SOMAXCONN) < 0) {
			close(this->serverSocket);
			this->serverSocket = -1;
			throw std::runtime_error("[TcpServer] listen() failed");
		}

		this->isRunning.store(true);
		fmt::println("[TcpServer] Successfully started, listening on port {}", this->port);
	}


	void TcpServer::run() {
		if (!this->isRunning.load()) {
			throw std::runtime_error("[TcpServer] Server not started!");
		}

		while (this->isRunning.load()) {
			sockaddr_in clientAddress{};
			socklen_t clientLength = sizeof(clientAddress);

			int clientSocket = accept(this->serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientLength);
			if (clientSocket < 0) {
				if (this->isRunning.load()) {
					if (errno == EINTR) {
						continue;
					}
					perror("[TcpServer] accept");
				}
				continue;
			}

			uint32_t clientIp = clientAddress.sin_addr.s_addr;

			// cleanup finished connections and rate limit
			this->cleanupFinishedConnections();
			this->cleanupStaleRateLimitEntries();

			// check connection limit
			if (this->getActiveConnectionCount() >= this->maxConnections) {
				fmt::println("[TcpServer] Connection limit reached, rejecting client");
				close(clientSocket);
				continue;
			}

			// check rate limit
			if (this->isRateLimited(clientIp)) {
				char ipStr[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &clientIp, ipStr, sizeof(ipStr));
				fmt::println("[TcpServer] Rate limit exceeded for {}, rejecting", ipStr);
				close(clientSocket);
				continue;
			}

			auto conn = std::make_unique<ClientConnection>();
			ClientConnection* connPtr = conn.get();

			conn->thread = std::jthread([this, clientSocket, connPtr](std::stop_token stopToken) {
				this->handleClient(clientSocket, stopToken);
				close(clientSocket);
				connPtr->finished.store(true);
			});

			std::lock_guard lock(this->connectionsMutex);
			this->connections.push_back(std::move(conn));
		}
	}


	void TcpServer::cleanupFinishedConnections() {
		std::lock_guard lock(this->connectionsMutex);

		// remove connections where the threads already finished
		std::erase_if(this->connections, [](const std::unique_ptr<ClientConnection>& conn) {
			return conn->finished.load();
		});
	}


	size_t TcpServer::getActiveConnectionCount() {
		std::lock_guard lock(this->connectionsMutex);
		return std::count_if(this->connections.begin(), this->connections.end(),
			[](const std::unique_ptr<ClientConnection>& conn) {
				return !conn->finished.load();
			});
	}


	bool TcpServer::isRateLimited(uint32_t clientIp) {
		std::lock_guard lock(this->rateLimitMutex);

		auto now = std::chrono::steady_clock::now();
		auto cutoff = now - this->rateLimitWindow;

		auto& attempts = this->connectionAttempts[clientIp];

		// remove timestamps outside the window
		while (!attempts.empty() && attempts.front() < cutoff) {
			attempts.pop_front();
		}

		// check if over limit
		if (attempts.size() >= this->rateLimitMaxConnections) {
			return true;
		}

		// record this attempt
		attempts.push_back(now);
		return false;
	}


	void TcpServer::cleanupStaleRateLimitEntries() {
		std::lock_guard lock(this->rateLimitMutex);

		auto now = std::chrono::steady_clock::now();
		auto cutoff = now - this->rateLimitWindow;

		// remove IPs with no recent connection attempts
		std::erase_if(this->connectionAttempts, [cutoff](auto& entry) {
			auto& attempts = entry.second;
			// remove expired timestamps
			while (!attempts.empty() && attempts.front() < cutoff) {
				attempts.pop_front();
			}
			// remove entry if no timestamps remain
			return attempts.empty();
		});
	}


	void TcpServer::stop() {
		if (!this->isRunning.exchange(false)) {
			return;
		}

		if (this->serverSocket >= 0) {
			// unblock threads that wait in accept
			shutdown(this->serverSocket, SHUT_RDWR);
			close(this->serverSocket);
			this->serverSocket = -1;
		}

		// wait for all client threads to finish
		std::lock_guard lock(this->connectionsMutex);
		this->connections.clear();
	}


	void TcpServer::handleClient(int clientSocket, std::stop_token stopToken) {
		if (this->clientHandler) {
			this->clientHandler(clientSocket, stopToken);
			return;
		}

		// default if no callback is set: echo server
		char buffer[1024];

		while (!stopToken.stop_requested()) {
			ssize_t numBytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
			if (numBytesReceived <= 0) {
				break;
			}

			// send all received bytes:
			ssize_t totalBytesSent = 0;
			while (totalBytesSent < numBytesReceived && !stopToken.stop_requested()) {
				ssize_t sent = send(clientSocket, buffer + totalBytesSent, numBytesReceived - totalBytesSent, 0);
				if (sent <= 0) {
					return;
				}
				totalBytesSent += sent;
			}
		}
	}
} // namespace df::mp
