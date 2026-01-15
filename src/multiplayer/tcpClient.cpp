#include "tcpClient.h"

#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <fmt/base.h>
#include <netinet/in.h>
#include <stdexcept>
#include <span>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>




namespace df::mp {
	TcpClient::TcpClient() {
		fmt::println("[TcpClient] client created");
	}


	TcpClient::~TcpClient() {
		this->disconnect();
		fmt::println("[TcpClient] client destroyed");
	}


	void TcpClient::tryConnect(const std::string& serverAddress, uint16_t serverPort) {
		if (this->connected) {
			throw std::runtime_error("[TcpClient] Already connected. Call disconnect() first.");
		}

		this->serverAddress = serverAddress;
		this->serverPort = serverPort;

		// create a new socket for this connection
		this->tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (this->tcpSocket == INVALID_SOCKET) {
			throw std::runtime_error("[TcpClient] Failed to create socket.");
		}

		struct sockaddr_in server;
		std::memset(&server, 0, sizeof(server));
		server.sin_family = AF_INET;
		server.sin_port = htons(this->serverPort);

		if (inet_pton(AF_INET, this->serverAddress.c_str(), &server.sin_addr) <= 0) {
			close(this->tcpSocket);
			this->tcpSocket = INVALID_SOCKET;
			throw std::runtime_error("[TcpClient] Invalid IP address: " + this->serverAddress);
		}

		if (connect(this->tcpSocket, reinterpret_cast<sockaddr*>(&server), sizeof(server)) == SOCKET_ERROR) {
			close(this->tcpSocket);
			this->tcpSocket = INVALID_SOCKET;
			throw std::runtime_error("[TcpClient] Failed to connect to " + this->serverAddress + ":" + std::to_string(this->serverPort));
		}

		this->connected = true;
	}


	void TcpClient::sendAll(std::span<const std::byte> data) {
		size_t totalBytesSent = 0;

		// make sure all data is sent
		while (totalBytesSent < data.size()) {
			ssize_t numBytesSent = send(this->tcpSocket, data.data() + totalBytesSent, data.size() - totalBytesSent, 0);

			if (numBytesSent == SOCKET_ERROR) {
				throw std::runtime_error("[TcpClient] Failed to send data");
			}
			if (numBytesSent == 0) {
				throw std::runtime_error("[TcpClient] Connection closed by peer");
			}
			totalBytesSent += numBytesSent;
		}
	}


	void TcpClient::trySend(const std::string& data) {
		this->sendAll(std::as_bytes(std::span{data}));
	}


	void TcpClient::trySend(const std::vector<uint8_t>& data) {
		this->sendAll(std::as_bytes(std::span{data}));
	}


	std::string TcpClient::tryReceive(size_t bufferSize) {
		if (bufferSize == 0) {
			throw std::runtime_error("[TcpClient] Buffer size must be > 0");
		}

		std::vector<char> buffer(bufferSize);
		ssize_t numBytesRead = recv(this->tcpSocket, buffer.data(), bufferSize - 1, 0);

		if (numBytesRead == SOCKET_ERROR) {
			throw std::runtime_error("[TcpClient] Failed to receive data");
		}
		if (numBytesRead == 0) {
			throw std::runtime_error("[TcpClient] Connection closed by peer");
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


	void TcpClient::disconnect() noexcept {
		if (this->tcpSocket == INVALID_SOCKET) {
			return;
		}

		if (close(this->tcpSocket) == SOCKET_ERROR) {
			fmt::println("[TcpClient] Warning: failed to close socket");
		}

		this->tcpSocket = INVALID_SOCKET;
		this->connected = false;
	}


	bool TcpClient::isConnected() const noexcept {
		return this->connected;
	}
} // namespace df::mp
