#include "tcpServer.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fmt/base.h>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <stdexcept>
#include <stop_token>
#include <sys/socket.h>
#include <unistd.h>



namespace df::net {
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


	void TcpServer::onClientCallback(ClientHandler handler) {
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
		setsockopt(this->serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

			std::lock_guard lock(this->clientThreadsMutex);
			this->clientThreads.emplace_back([this, clientSocket](std::stop_token) {
				this->handleClient(clientSocket);
				close(clientSocket);
			});
		}
	}


	void TcpServer::stop() {
		if (!this->isRunning.exchange(false)) {
			return;
		}

		if (this->serverSocket >= 0) {
			close(this->serverSocket);
			this->serverSocket = -1;
		}

		std::lock_guard lock(this->clientThreadsMutex);
		this->clientThreads.clear();
	}


	void TcpServer::handleClient(int clientSocket) {
		if (this->clientHandler) {
			this->clientHandler(clientSocket);
			return;
		}

		// default if no callback is set: echo server
		// make buffer configurable
		char buffer[1024];

		while (true) {
			ssize_t numBytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
			if (numBytesReceived <= 0) {
				break;
			}

			// send all received bytes:
			ssize_t totalBytesSent = 0;
			while (totalBytesSent < numBytesReceived) {
				ssize_t sent = send(clientSocket, buffer + totalBytesSent, numBytesReceived - totalBytesSent, 0);
				if (sent <= 0) {
					return;
				}
				totalBytesSent += sent;
			}
		}
	}
} // namespace df::net
