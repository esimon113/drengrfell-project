#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <thread>
#include <vector>


namespace df::mp {
	class TcpServer {
	  public:
		using ClientHandler = std::function<void(int)>;

		// use singleton pattern -> there should only be one server at a time
		static TcpServer& instance();

		void configure(uint16_t port, const std::string& bindTo = "0.0.0.0");
		void start();
		void run();
		void stop();

		// called on a separate thread fro each client.
		// This shoudl be used to implement the protocol used by the game.
		// After it finishes, the socket is closed.
		void onClientCallback(ClientHandler handler);

		// disable copying/moving
		TcpServer(const TcpServer&) = delete;
		TcpServer& operator=(const TcpServer&) = delete;


	  private:
		TcpServer();
		~TcpServer();

		void handleClient(int clientSocket);

		int port{0};
		int serverSocket{-1};
		sockaddr_in serverAddress{};

		ClientHandler clientHandler;

		std::atomic<bool> isRunning{false};

		std::mutex clientThreadsMutex;
		std::vector<std::jthread> clientThreads;
	};
} // namespace df::mp
