#include "fmt/base.h"
// #include <application.h>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <thread>
// #include <utils/commandLineOptions.h>

#include <iostream>


void print(std::string s) {
	std::cout << s << std::endl;
}


// int _main(int argc, char** argv) {
// 	print("Starting and trying to initialize app...");
//
// 	df::CommandLineOptions options = df::CommandLineOptions::parse(argc, argv);
// 	std::optional<df::Application> app = df::Application::init(options);
//
//
// 	if (!app) {
// 		return EXIT_FAILURE;
// 	}
// 	std::cout << "Test0!" << std::endl;
// 	print("Try running app...");
// 	app->run();
// 	std::cout << "Test1" << std::endl;
//
// 	print("Try deinitialize app...");
// 	app->deinit();
//
// 	print("Done.");
//
// 	return EXIT_SUCCESS;
// }



// TODO: Remove this! -> Only for TESTING!
#include "multiplayer/asgard.h"
#include "multiplayer/midgard.h"

int main(int /*argc*/, char** /*argv*/) {
	using namespace std::chrono_literals;

	constexpr uint16_t kPort = 45678;
	constexpr const char* kHost = "127.0.0.1";

	// Start the authoritative server
	df::bifrost::Asgard server;
	server.configure(kPort, kHost);
	server.start();
	fmt::println("[Test] Asgard server started on {}:{}", kHost, kPort);

	// Create two clients (minPlayers = 2)
	df::bifrost::Midgard clientA;
	df::bifrost::Midgard clientB;

	std::atomic<bool> gameStarted{false};

	auto installCallbacks = [](df::bifrost::Midgard& client, const std::string& label) {
		client.setConnectionCallback([label](bool connected, const std::string& reason) {
			fmt::println("[{}] Connection: {} ({})", label, connected ? "connected" : "disconnected", reason);
		});
		client.setLobbyStateCallback([label](const df::bifrost::LobbyState& lobby) {
			fmt::println("[{}] LobbyState: players={}, hostId={}", label, lobby.players.size(), lobby.hostId);
		});
		client.setActionResultCallback([label](uint32_t seq, bool success, const std::optional<df::bifrost::ErrorInfo>& error) {
			if (!success && error) {
				fmt::println("[{}] ActionResult seq={} failed: {} ({})", label, seq, error->message, df::bifrost::errorCodeToString(error->code));
			} else {
				fmt::println("[{}] ActionResult seq={} success", label, seq);
			}
		});
		client.setGameStateCallback([label](const nlohmann::json& state) {
			fmt::println("[{}] GameState received (size={} keys)", label, state.size(), state.is_object() ? state.size() : 0);
		});
	};

	installCallbacks(clientA, "ClientA");
	installCallbacks(clientB, "ClientB");

	auto connectWithRetry = [&](df::bifrost::Midgard& client, const std::string& label) {
		for (int attempt = 1; attempt <= 10; ++attempt) {
			if (client.connect(kHost, kPort)) {
				return true;
			}
			fmt::println("[Test] {} connect retry {}/10...", label, attempt);
			std::this_thread::sleep_for(100ms);
		}
		return false;
	};

	// Give the server a brief moment to start listening
	std::this_thread::sleep_for(200ms);

	if (!connectWithRetry(clientA, "ClientA") || !connectWithRetry(clientB, "ClientB")) {
		fmt::println("[Test] Failed to connect clients");
		server.stop();
		return EXIT_FAILURE;
	}

	clientA.setGameEventCallback([&](const df::bifrost::Message& msg) {
		if (msg.type == df::bifrost::MessageType::GAME_STARTED) {
			gameStarted.store(true);
		}
	});

	// Join lobby
	clientA.join("VikingA");
	clientB.join("VikingB");
	std::this_thread::sleep_for(200ms);

	// Ready up and start
	clientA.setReady(true);
	clientB.setReady(true);
	std::this_thread::sleep_for(200ms);
	clientA.startGame();

	// Wait for game start or timeout
	for (int i = 0; i < 20 && !gameStarted.load(); ++i) {
		std::this_thread::sleep_for(100ms);
	}

	fmt::println("[Test] Game started: {}", gameStarted.load() ? "yes" : "no");

	// Basic ping test
	clientA.ping();
	clientB.ping();
	std::this_thread::sleep_for(200ms);

	// Clean up
	clientA.leave();
	clientB.leave();
	std::this_thread::sleep_for(200ms);
	server.stop();

	fmt::println("[Test] Done");
	return EXIT_SUCCESS;
}
