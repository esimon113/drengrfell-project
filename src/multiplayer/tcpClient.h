#pragma once

#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <netinet/in.h>
#include <span>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <vector>

inline constexpr int INVALID_SOCKET = -1;
inline constexpr int SOCKET_ERROR = -1;



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

		int tcpSocket{INVALID_SOCKET};
		bool connected{false};
		std::string serverAddress;
		uint16_t serverPort{0};
	};
} // namespace df::mp
