#include "tcpServer.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fmt/base.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <stop_token>



namespace df::mp {
	TcpServer& TcpServer::instance() {
		static TcpServer srv;
		return srv;
	}


	TcpServer::TcpServer() = default;


	TcpServer::~TcpServer() {
		this->stop();
	}


	void TcpServer::configure(uint16_t portValue, const std::string& bindTo) {
		if (this->isRunning.load()) {
			throw std::runtime_error("[TcpServer] Cannot configure the server while running!");
		}
		this->port = portValue;

		this->serverAddress = net::makeIpv4Address(bindTo, this->port);
	}


	void TcpServer::setMaxConnections(size_t maxConnectionsValue) {
		if (this->isRunning.load()) {
			throw std::runtime_error("[TcpServer] Cannot configure while running!");
		}
		this->maxConnections = maxConnectionsValue;
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

		this->serverSocket = net::createTcpSocket();
		if (!net::isValid(this->serverSocket)) {
			throw std::runtime_error("[TcpServer] Creating a socket failed!");
		}

		if (!net::setReuseAddr(this->serverSocket, true)) {
			net::close(this->serverSocket);
			this->serverSocket = net::INVALID_SOCKET_HANDLE;
			throw std::runtime_error("[TcpServer] setsockopt(SO_REUSEADDR) failed");
		}

		if (!net::bind(this->serverSocket, this->serverAddress)) {
			net::close(this->serverSocket);
			this->serverSocket = net::INVALID_SOCKET_HANDLE;
			throw std::runtime_error("[TcpServer] bind() failed");
		}

		if (!net::listen(this->serverSocket, SOMAXCONN)) {
			net::close(this->serverSocket);
			this->serverSocket = net::INVALID_SOCKET_HANDLE;
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
			net::SocketAddress clientAddress{};
			net::SocketHandle clientSocket = net::accept(this->serverSocket, clientAddress);
			if (!net::isValid(clientSocket)) {
				if (this->isRunning.load()) {
					const int err = net::lastError();
					if (net::isInterruptedError(err)) {
						continue;
					}
					fmt::println("[TcpServer] accept failed: {}", net::errorMessage(err));
				}
				continue;
			}

			uint32_t clientIp = net::getIpv4Address(clientAddress);

			// cleanup finished connections and rate limit
			this->cleanupFinishedConnections();
			this->cleanupStaleRateLimitEntries();

			// check connection limit
			if (this->getActiveConnectionCount() >= this->maxConnections) {
				fmt::println("[TcpServer] Connection limit reached, rejecting client");
				net::close(clientSocket);
				continue;
			}

			// check rate limit
			if (this->isRateLimited(clientIp)) {
				fmt::println("[TcpServer] Rate limit exceeded for {}, rejecting", net::ipv4ToString(clientIp));
				net::close(clientSocket);
				continue;
			}

			auto conn = std::make_unique<ClientConnection>();
			ClientConnection* connPtr = conn.get();

			conn->thread = std::jthread([this, clientSocket, connPtr](std::stop_token stopToken) {
				this->handleClient(clientSocket, stopToken);
				net::close(clientSocket);
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

		if (net::isValid(this->serverSocket)) {
			// unblock threads that wait in accept
			net::shutdownBoth(this->serverSocket);
			net::close(this->serverSocket);
			this->serverSocket = net::INVALID_SOCKET_HANDLE;
		}

		// wait for all client threads to finish
		std::lock_guard lock(this->connectionsMutex);
		this->connections.clear();
	}


	void TcpServer::handleClient(net::SocketHandle clientSocket, std::stop_token stopToken) {
		if (this->clientHandler) {
			this->clientHandler(clientSocket, stopToken);
			return;
		}

		// default if no callback is set: echo server
		char buffer[1024];

		while (!stopToken.stop_requested()) {
			int numBytesReceived = net::recv(clientSocket, buffer, sizeof(buffer));
			if (numBytesReceived <= 0) {
				break;
			}

			// send all received bytes:
			int totalBytesSent = 0;
			while (totalBytesSent < numBytesReceived && !stopToken.stop_requested()) {
				int sent = net::send(clientSocket, buffer + totalBytesSent, static_cast<size_t>(numBytesReceived - totalBytesSent));
				if (sent <= 0) {
					return;
				}
				totalBytesSent += sent;
			}
		}
	}
} // namespace df::mp
