#include "multiplayer/network/socketPlatform.h"
#include "multiplayer/network/tcpClient.h"
#include "multiplayer/network/tcpServer.h"

#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {
	using namespace std::chrono_literals;

	struct TestContext {
		int failures = 0;
	};

	void assertTrue(TestContext& ctx, bool condition, const std::string& message) {
		if (!condition) {
			++ctx.failures;
			std::cerr << "[FAIL] " << message << '\n';
		}
	}

	void logInfo(const std::string& message) {
		std::cout << "[INFO] " << message << '\n';
	}

	void runTest(TestContext& ctx, const std::string& name, const std::function<void()>& testFn) {
		std::cout << "[TEST] " << name << '\n';
		try {
			testFn();
		} catch (const std::exception& ex) {
			++ctx.failures;
			std::cerr << "[FAIL] " << name << ": " << ex.what() << '\n';
		} catch (...) {
			++ctx.failures;
			std::cerr << "[FAIL] " << name << ": unknown exception\n";
		}
	}

	uint16_t findAvailablePort() {
		df::mp::net::SocketPlatform platform;
		auto socket = df::mp::net::createTcpSocket();
		if (!df::mp::net::isValid(socket)) {
			throw std::runtime_error("Failed to create port probe socket");
		}
		logInfo("Port probe socket created");

		auto address = df::mp::net::makeIpv4Address("127.0.0.1", 0);
		if (!df::mp::net::bind(socket, address)) {
			df::mp::net::close(socket);
			throw std::runtime_error("Failed to bind port probe socket");
		}
		logInfo("Port probe socket bound to 127.0.0.1:0");

		if (!df::mp::net::listen(socket, 1)) {
			df::mp::net::close(socket);
			throw std::runtime_error("Failed to listen on port probe socket");
		}
		logInfo("Port probe socket listening");

		sockaddr_in actual{};
#ifdef _WIN32
		int length = sizeof(actual);
#else
		socklen_t length = sizeof(actual);
#endif
		if (::getsockname(socket, reinterpret_cast<sockaddr*>(&actual), &length) != 0) {
			df::mp::net::close(socket);
			throw std::runtime_error("Failed to get probe socket address");
		}

		uint16_t port = ntohs(actual.sin_port);
		logInfo("Selected port: " + std::to_string(port));
		df::mp::net::close(socket);
		return port;
	}

	class ServerRunner {
	  public:
		ServerRunner(const std::function<void(df::mp::net::SocketHandle, std::stop_token)>& handler,
			size_t maxConnections = 5,
			size_t rateLimitMax = 100,
			std::chrono::seconds rateWindow = std::chrono::seconds(60)) :
			port(findAvailablePort()) {
			auto& server = df::mp::TcpServer::instance();
			server.configure(port, "127.0.0.1");
			server.setMaxConnections(maxConnections);
			server.setRateLimit(rateLimitMax, rateWindow);
			server.onClientCallback(handler);
			server.start();
			logInfo("Server started on 127.0.0.1:" + std::to_string(port));

			thread = std::jthread([](std::stop_token) {
				df::mp::TcpServer::instance().run();
			});

			std::this_thread::sleep_for(50ms);
		}

		~ServerRunner() {
			df::mp::TcpServer::instance().stop();
		}

		uint16_t port;

	  private:
		std::jthread thread;
	};

	// void expectThrows(TestContext& ctx, const std::string& message, const std::function<void()>& fn) {
	// 	bool thrown = false;
	// 	try {
	// 		fn();
	// 	} catch (...) {
	// 		thrown = true;
	// 	}
	// 	assertTrue(ctx, thrown, message);
	// }
} // namespace

int main() {
	TestContext ctx;

	runTest(ctx, "TcpClient echo string", [&ctx]() {
		auto handler = [](df::mp::net::SocketHandle socket, std::stop_token) {
			char buffer[64]{};
			int bytes = df::mp::net::recv(socket, buffer, sizeof(buffer));
			if (bytes > 0) {
				df::mp::net::send(socket, buffer, static_cast<size_t>(bytes));
			}
		};

		ServerRunner server(handler);

		df::mp::TcpClient client;
		logInfo("Echo string: connecting client");
		client.tryConnect("127.0.0.1", server.port);
		logInfo("Echo string: connected, sending ping");
		client.trySend(std::string("ping"));
		logInfo("Echo string: awaiting reply");
		auto reply = client.tryReceive(64);
		logInfo("Echo string: received '" + reply + "'");
		assertTrue(ctx, reply == "ping", "Echo reply should match");
		client.disconnect();
	});

	runTest(ctx, "TcpClient echo binary", [&ctx]() {
		auto handler = [](df::mp::net::SocketHandle socket, std::stop_token) {
			uint8_t buffer[8]{};
			int bytes = df::mp::net::recv(socket, buffer, sizeof(buffer));
			if (bytes > 0) {
				df::mp::net::send(socket, buffer, static_cast<size_t>(bytes));
			}
		};

		ServerRunner server(handler);

		df::mp::TcpClient client;
		logInfo("Echo binary: connecting client");
		client.tryConnect("127.0.0.1", server.port);
		std::vector<uint8_t> payload{1, 2, 3, 4};
		logInfo("Echo binary: sending payload size " + std::to_string(payload.size()));
		client.trySend(payload);
		logInfo("Echo binary: awaiting reply");
		auto reply = client.tryReceiveBinary(16);
		logInfo("Echo binary: received size " + std::to_string(reply.size()));
		assertTrue(ctx, reply == payload, "Binary echo reply should match");
		client.disconnect();
	});

	runTest(ctx, "TcpClient connection lifecycle", [&ctx]() {
		auto handler = [](df::mp::net::SocketHandle socket, std::stop_token) {
			const char ok[] = "ok";
			df::mp::net::send(socket, ok, 2);
		};

		ServerRunner server(handler);

		df::mp::TcpClient client;
		logInfo("Lifecycle: connecting client");
		client.tryConnect("127.0.0.1", server.port);
		assertTrue(ctx, client.isConnected(), "Client should be connected");
		logInfo("Lifecycle: disconnecting client");
		client.disconnect();
		assertTrue(ctx, !client.isConnected(), "Client should be disconnected");
		logInfo("Lifecycle: reconnecting client");
		client.tryConnect("127.0.0.1", server.port);
		assertTrue(ctx, client.isConnected(), "Client should reconnect");
		client.disconnect();
	});

	runTest(ctx, "TcpServer max connections", [&ctx]() {
		auto handler = [](df::mp::net::SocketHandle socket, std::stop_token) {
			const char ok[] = "ok";
			df::mp::net::send(socket, ok, 2);
			std::this_thread::sleep_for(200ms);
		};

		ServerRunner server(handler, 1);

		df::mp::TcpClient clientA;
		logInfo("Max connections: connecting clientA");
		clientA.tryConnect("127.0.0.1", server.port);
		auto replyA = clientA.tryReceive(8);
		assertTrue(ctx, replyA == "ok", "Primary connection should succeed");

		df::mp::TcpClient clientB;
		bool rejected = false;
		try {
			logInfo("Max connections: connecting clientB (should be rejected)");
			clientB.tryConnect("127.0.0.1", server.port);
			try {
				clientB.tryReceive(8);
			} catch (...) {
				logInfo("Max connections: clientB rejected on receive");
				rejected = true;
			}
		} catch (...) {
			logInfo("Max connections: clientB rejected on connect");
			rejected = true;
		}
		assertTrue(ctx, rejected, "Secondary connection should be rejected");

		clientA.disconnect();
		clientB.disconnect();
	});

	runTest(ctx, "TcpServer rate limiting", [&ctx]() {
		auto handler = [](df::mp::net::SocketHandle socket, std::stop_token) {
			const char ok[] = "ok";
			df::mp::net::send(socket, ok, 2);
		};

		ServerRunner server(handler, 5, 1, std::chrono::seconds(1));

		// Ensure any previous attempts in the singleton are outside the window.
		logInfo("Rate limit: waiting to clear prior attempts");
		std::this_thread::sleep_for(1100ms);

		df::mp::TcpClient clientA;
		logInfo("Rate limit: connecting clientA");
		clientA.tryConnect("127.0.0.1", server.port);
		auto replyA = clientA.tryReceive(8);
		assertTrue(ctx, replyA == "ok", "First connection should succeed");
		logInfo("Rate limit: disconnecting clientA");
		clientA.disconnect();

		df::mp::TcpClient clientB;
		bool rejected = false;
		try {
			logInfo("Rate limit: connecting clientB (should be rejected)");
			clientB.tryConnect("127.0.0.1", server.port);
			std::this_thread::sleep_for(50ms);
			try {
				clientB.tryReceive(8);
			} catch (...) {
				logInfo("Rate limit: clientB rejected on receive");
				rejected = true;
			}
		} catch (...) {
			logInfo("Rate limit: clientB rejected on connect");
			rejected = true;
		}
		assertTrue(ctx, rejected, "Rate limited connection should be rejected");
		clientB.disconnect();
	});

	if (ctx.failures == 0) {
		std::cout << "[OK] All network tests passed.\n";
		return 0;
	}

	std::cerr << "[FAIL] " << ctx.failures << " test(s) failed.\n";
	return 1;
}


