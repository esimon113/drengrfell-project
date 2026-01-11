#include "tcpClient.h"

#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <fmt/base.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>




namespace df::mp {
	TcpClient::TcpClient() : ipAddress("0.0.0.0"), port(56789), tcpSocket(INVALID_SOCKET), isSocketInitialized(false) {
		fmt::println("[TcpClient] created on {}:{}", this->ipAddress, this->port);
		this->initializeSocket();

		this->tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (this->tcpSocket == INVALID_SOCKET) {
			this->cleanupSocket();
			throw std::runtime_error("[TcpClient] Failed to create socket.");
		}
	}


	TcpClient::~TcpClient() {
		fmt::println("[TcpClient] client destroyed");
		this->disconnect();
		this->cleanupSocket();
	}


	void TcpClient::tryConnect(std::string& serverAddress, int serverPort) {
		this->serverAddress = serverAddress;
		this->serverPort = serverPort;

		struct sockaddr_in server;
		// cleanly init server memory
		std::memset(&server, 0, sizeof(server));
		server.sin_family = AF_INET;
		server.sin_port = htons(this->serverPort);

		if (inet_pton(AF_INET, this->serverAddress.c_str(), &server.sin_addr) <= 0) {
			server.sin_addr.s_addr = inet_addr(this->serverAddress.c_str());

			if (server.sin_addr.s_addr == INADDR_NONE) {
				throw std::runtime_error("[TcpClient] Invalid IP address: " + this->serverAddress);
			}
		}

		if (connect(this->tcpSocket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
			throw std::runtime_error("[TcpClient] Failed to connect to the server " + this->serverAddress + ":" + std::to_string(this->serverPort));
		}
	}


	void TcpClient::trySend(std::string& data) {
		ssize_t numBytesSent = send(this->tcpSocket, data.c_str(), data.size(), 0);

		if (numBytesSent == SOCKET_ERROR) {
			throw std::runtime_error("[TcpClient] Failed to send data");
		}
	}


	void TcpClient::trySend(const std::vector<uint8_t>& data) {
		ssize_t numBytesSent = send(this->tcpSocket, data.data(), data.size(), 0);

		if (numBytesSent == SOCKET_ERROR) {
			throw std::runtime_error("[TcpClient] Failed to send binary data");
		}
	}


	std::string TcpClient::tryReceive(size_t bufferSize) {
		std::vector<char> buffer(bufferSize);
		// bufferSize-1 because of '\0'
		ssize_t numBytesRead = recv(this->tcpSocket, buffer.data(), bufferSize - 1, 0);

		if (numBytesRead == SOCKET_ERROR) {
			throw std::runtime_error("[TcpClient] Failed to receive data");
		}

		buffer[numBytesRead] = '\0';
		return std::string(buffer.data());
	}


	std::vector<uint8_t> TcpClient::tryReceiveBinary(size_t bufferSize) {
		std::vector<uint8_t> buffer(bufferSize);
		ssize_t numBytesRead = recv(this->tcpSocket, buffer.data(), bufferSize, 0);

		if (numBytesRead == SOCKET_ERROR) {
			throw std::runtime_error("[TcpClient] Failed to receive binary data");
		}
		if (numBytesRead == 0) {
			throw std::runtime_error("[TcpClient] Connection closed by peer");
		}

		buffer.resize(numBytesRead);
		return buffer;
	}


	void TcpClient::disconnect() {
		if (this->tcpSocket == INVALID_SOCKET) {
			return;
		}

		if (close(this->tcpSocket) == SOCKET_ERROR) {
			throw std::runtime_error("[TcpClient] Failed to close socket");
		}

		this->tcpSocket = INVALID_SOCKET;
	}
} // namespace df::mp
