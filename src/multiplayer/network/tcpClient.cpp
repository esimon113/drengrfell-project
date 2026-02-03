#include "tcpClient.h"

#include <cstddef>
#include <cstring>
#include <fmt/base.h>
#include <stdexcept>
#include <span>
#include <string>




namespace df::mp {
	TcpClient::TcpClient() {
		fmt::println("[TcpClient] client created");
	}


	TcpClient::~TcpClient() {
		this->disconnect();
		fmt::println("[TcpClient] client destroyed");
	}


	void TcpClient::tryConnect(const std::string& serverAddressInput, uint16_t serverPortInput) {
		if (this->connected) {
			throw std::runtime_error("[TcpClient] Already connected. Call disconnect() first.");
		}

		this->serverAddress = serverAddressInput;
		this->serverPort = serverPortInput;

		// create a new socket for this connection
		this->tcpSocket = net::createTcpSocket();
		if (!net::isValid(this->tcpSocket)) {
			throw std::runtime_error("[TcpClient] Failed to create socket.");
		}

		const auto serverEndpoint = net::makeIpv4Address(this->serverAddress, this->serverPort);
		if (!net::connect(this->tcpSocket, serverEndpoint)) {
			net::close(this->tcpSocket);
			this->tcpSocket = net::INVALID_SOCKET_HANDLE;
			throw std::runtime_error("[TcpClient] Failed to connect to " + this->serverAddress + ":" + std::to_string(this->serverPort));
		}

		this->connected = true;
	}


	void TcpClient::sendAll(std::span<const std::byte> data) {
		size_t totalBytesSent = 0;

		// make sure all data is sent
		while (totalBytesSent < data.size()) {
			int numBytesSent = net::send(this->tcpSocket, data.data() + totalBytesSent, data.size() - totalBytesSent);

			if (numBytesSent == net::SOCKET_ERROR_CODE) {
				throw std::runtime_error("[TcpClient] Failed to send data");
			}
			if (numBytesSent == 0) {
				throw std::runtime_error("[TcpClient] Connection closed by peer");
			}
			totalBytesSent += static_cast<size_t>(numBytesSent);
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
		int numBytesRead = net::recv(this->tcpSocket, buffer.data(), bufferSize - 1);

		if (numBytesRead == net::SOCKET_ERROR_CODE) {
			throw std::runtime_error("[TcpClient] Failed to receive data");
		}
		if (numBytesRead == 0) {
			throw std::runtime_error("[TcpClient] Connection closed by peer");
		}

		buffer[static_cast<size_t>(numBytesRead)] = '\0';
		return std::string(buffer.data());
	}


	std::vector<uint8_t> TcpClient::tryReceiveBinary(size_t bufferSize) {
		std::vector<uint8_t> buffer(bufferSize);
		int numBytesRead = net::recv(this->tcpSocket, buffer.data(), bufferSize);

		if (numBytesRead == net::SOCKET_ERROR_CODE) {
			throw std::runtime_error("[TcpClient] Failed to receive binary data");
		}
		if (numBytesRead == 0) {
			throw std::runtime_error("[TcpClient] Connection closed by peer");
		}

		buffer.resize(static_cast<size_t>(numBytesRead));
		return buffer;
	}


	void TcpClient::disconnect() noexcept {
		if (!net::isValid(this->tcpSocket)) {
			return;
		}

		net::close(this->tcpSocket);
		this->tcpSocket = net::INVALID_SOCKET_HANDLE;
		this->connected = false;
	}


	bool TcpClient::isConnected() const noexcept {
		return this->connected;
	}
} // namespace df::mp
