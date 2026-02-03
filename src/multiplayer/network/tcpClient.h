#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "multiplayer/network/socketPlatform.h"



namespace df::mp {
	class TcpClient {
	  public:
		TcpClient();
		~TcpClient();

		void tryConnect(const std::string& serverAddress, uint16_t serverPort);
		void disconnect() noexcept;
		[[nodiscard]] bool isConnected() const noexcept;

		void trySend(const std::string& data);
		void trySend(const std::vector<uint8_t>& data);

		std::string tryReceive(size_t bufferSize);
		std::vector<uint8_t> tryReceiveBinary(size_t bufferSize);


	  private:
		void sendAll(std::span<const std::byte> data);

		net::SocketPlatform socketPlatform;
		net::SocketHandle tcpSocket{net::INVALID_SOCKET_HANDLE};
		bool connected{false};
		std::string serverAddress;
		uint16_t serverPort{0};
	};
} // namespace df::mp
