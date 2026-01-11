#pragma once

#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <vector>

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1



namespace df::mp {
	class TcpClient {
	  public:
		TcpClient();
		~TcpClient();

		void tryConnect(std::string& serverAddress, int serverPort);
		void disconnect();

		void trySend(std::string& data);
		void trySend(const std::vector<uint8_t>& data);

		std::string tryReceive(size_t bufferSize);
		std::vector<uint8_t> tryReceiveBinary(size_t bufferSize);


	  private:
		void initializeSocket();
		void cleanupSocket();

		std::string ipAddress;
		int port;
		int tcpSocket;
		std::string serverAddress;
		int serverPort;

		bool isSocketInitialized;
	};
} // namespace df::mp
