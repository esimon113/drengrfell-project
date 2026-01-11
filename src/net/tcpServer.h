#pragma once

#include "fmt/format.h"
#include <atomic>
#include <cstdint>
#include <netinet/in.h>



namespace df::net {
	class TcpServer {
	  public:
		// use singleton pattern -> there should only be one server at a time
		static TcpServer& instance();

		void configure(uint16_t port);
		void start();
		void run();
		void stop();

		// disable copying/moving
		TcpServer(const TcpServer&) = delete;
		TcpServer& operator=(const TcpServer&) = delete;
		TcpServer(TcpServer&&) = delete;
		TcpServer& operator=(TcpServer&&) = delete;


	  private:
		TcpServer();
		~TcpServer();

		void handleClient(int clientSocket);

		int port{0};
		int serverSocket{-1};
		sockaddr_in serverAddress{};

		std::atomic<bool> isRunning{false};
	};
} // namespace df::net
