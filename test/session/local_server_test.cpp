#include "multiplayer/asgard.h"
#include "multiplayer/midgard.h"
#include "multiplayer/network/socketPlatform.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
	using namespace std::chrono_literals;

	uint16_t findAvailablePort() {
		auto socket = df::mp::net::createTcpSocket();
		if (!df::mp::net::isValid(socket)) {
			throw std::runtime_error("port probe socket failed");
		}
		const auto address = df::mp::net::makeIpv4Address("127.0.0.1", 0);
		if (!df::mp::net::bind(socket, address) || !df::mp::net::listen(socket, 1)) {
			df::mp::net::close(socket);
			throw std::runtime_error("port probe bind failed");
		}
		sockaddr_in actual{};
#ifdef _WIN32
		int length = sizeof(actual);
#else
		socklen_t length = sizeof(actual);
#endif
		if (::getsockname(socket, reinterpret_cast<sockaddr*>(&actual), &length) != 0) {
			df::mp::net::close(socket);
			throw std::runtime_error("port probe getsockname failed");
		}
		df::mp::net::close(socket);
		return ntohs(actual.sin_port);
	}

	bool waitFor(const std::function<bool()>& condition, std::chrono::milliseconds limit) {
		const auto deadline = std::chrono::steady_clock::now() + limit;
		while (std::chrono::steady_clock::now() < deadline) {
			if (condition()) {
				return true;
			}
			std::this_thread::sleep_for(20ms);
		}
		return condition();
	}

	int fail(const std::string& message) {
		std::cerr << "local_server_test: " << message << '\n';
		return EXIT_FAILURE;
	}
}

int main() {
	df::mp::net::SocketPlatform platform;

	std::atomic<bool> finished{false};
	std::thread watchdog([&finished] {
		const auto deadline = std::chrono::steady_clock::now() + 60s;
		while (!finished && std::chrono::steady_clock::now() < deadline) {
			std::this_thread::sleep_for(50ms);
		}
		if (!finished) {
			std::cerr << "local_server_test: timed out, something is stuck\n";
			std::_Exit(EXIT_FAILURE);
		}
	});
	watchdog.detach();

	const uint16_t port = findAvailablePort();
	df::bifrost::Asgard server;
	server.configure(port, "127.0.0.1");

	{
		auto occupant = df::mp::net::createTcpSocket();
		df::mp::net::setReuseAddr(occupant, true);
		if (!df::mp::net::bind(occupant, df::mp::net::makeIpv4Address("0.0.0.0", port)) ||
			!df::mp::net::listen(occupant, 1)) {
			df::mp::net::close(occupant);
			return fail("could not occupy the test port");
		}
		const bool startedOnBusyPort = server.start();
		df::mp::net::close(occupant);
#ifndef _WIN32
		if (startedOnBusyPort || server.isRunning()) {
			return fail("server reported a start on a port another server listens on");
		}
#else
		if (startedOnBusyPort) {
			server.stop();
		}
#endif
	}

	const unsigned entityBefore = Entity();
	if (!server.start() || !server.isRunning()) {
		return fail("server did not start on a free port");
	}

	std::atomic<bool> gotState{false};
	{
		df::bifrost::Midgard client;
		client.setGameStateCallback([&gotState](const nlohmann::json& state) {
			if (state.contains("players")) {
				gotState = true;
			}
		});
		if (!client.connect("127.0.0.1", port)) {
			return fail("client could not connect to the server in the same process");
		}
		client.join("Solo");
		if (!waitFor([&client] { return client.getPlayerId().has_value(); }, 3000ms)) {
			return fail("join was not answered");
		}

		df::bifrost::LobbyConfig config;
		config.solo = true;
		client.updateConfig(config);
		client.setReady(true);
		client.startGame();
		if (!waitFor([&gotState] { return gotState.load(); }, 20000ms)) {
			return fail("solo match did not start");
		}

		if (Entity() != entityBefore + 1) {
			return fail("server created ECS entities, which races with the window's own entities");
		}

		const auto before = std::chrono::steady_clock::now();
		client.disconnect();
		if (std::chrono::steady_clock::now() - before > 2s) {
			return fail("disconnect took longer than 2 seconds");
		}
		if (client.isConnected()) {
			return fail("client still connected after disconnect");
		}
	}

	df::bifrost::Midgard lingering;
	if (!lingering.connect("127.0.0.1", port)) {
		return fail("second window could not connect");
	}
	lingering.join("Late");
	std::this_thread::sleep_for(200ms);

	const auto beforeStop = std::chrono::steady_clock::now();
	server.stop();
	if (std::chrono::steady_clock::now() - beforeStop > 2s) {
		return fail("stop waited for a window that was still connected");
	}
	if (server.isRunning()) {
		return fail("server still running after stop");
	}
	if (!waitFor([&lingering] { return !lingering.isConnected(); }, 2000ms)) {
		return fail("still-connected window did not notice the server stopped");
	}

	finished = true;
	std::cout << "local_server_test: start, busy port, solo match, disconnect, stop OK\n";
	return EXIT_SUCCESS;
}
