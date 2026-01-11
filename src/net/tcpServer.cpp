#include "tcpServer.h"

#include <asm-generic/socket.h>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

#include <fmt/base.h>



namespace df::net {
	TcpServer& TcpServer::instance() {
		static TcpServer srv;
		return srv;
	}

	TcpServer::TcpServer() = default;

	TcpServer::~TcpServer() {
		this->stop();
	}

	void TcpServer::configure(uint16_t port) {
		if (this->isRunning) {
			std::cerr << "Cannot configure the server while running!" << std::endl;
			return;
		}
		this->port = port;
	}

	void TcpServer::start() {
		if (this->isRunning) {
			return;
		}

		if (this->port == 0) {
			throw std::runtime_error("Port is not configured!");
		}

		this->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (this->serverSocket < 0) {
			throw std::runtime_error("[TcpServer] Creating a socket failed!");
			int opt = 1;
			setsockopt(this->serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
			this->serverAddress.sin_family = AF_INET;
			this->serverAddress.sin_addr.s_addr = INADDR_ANY;
			this->serverAddress.sin_port = htons(this->port);

			if (bind(this->serverSocket, (sockaddr*)&this->serverAddress, sizeof(this->serverAddress)) < 0) {
				close(this->serverSocket);
				throw std::runtime_error("[TcpServer] bind() failed");
			}

			if (listen(this->serverSocket, SOMAXCONN) < 0) {
				close(this->serverSocket);
				throw std::runtime_error("[TcpServer] listen() failed");
			}

			this->isRunning = true;
			fmt::println("Successfully started the TcpServer. Listening on port %d", this->port);
		}
	}

	void TcpServer::run() {
		if (!this->isRunning) {
			throw std::runtime_error("[TcpServer] Server is not started!");
		}

		while (this->isRunning) {
			sockaddr_in clientAddress{};
			socklen_t clientLength = sizeof(clientAddress);

			int clientSocket = accept(this->serverSocket, (sockaddr*)&clientAddress, &clientLength);
			if (clientSocket < 0) {
				if (this->isRunning) {
					perror("accept");
				}
				continue;
			}
			this->handleClient();
			close(clientSocket);
		}
	}

	void TcpServer::stop() {
		if (!this->isRunning) {
			return;
		}

		this->isRunning = false;

		if (this->serverSocket >= 0) {
			close(this->serverSocket);
			this->serverSocket = -1;
		}
	}

	void handleClient(int /*clientSocket*/) {
		// what happens if client connect
		// solve this with callbacks (like in mosquitto)?!
	}
} // namespace df::net
