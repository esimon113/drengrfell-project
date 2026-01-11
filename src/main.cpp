#include <application.h>
#include <cstdlib>
#include <utils/commandLineOptions.h>

#include <iostream>


void print(std::string s) {
	std::cout << s << std::endl;
}


int _main(int argc, char** argv) {
	print("Starting and trying to initialize app...");

	df::CommandLineOptions options = df::CommandLineOptions::parse(argc, argv);
	std::optional<df::Application> app = df::Application::init(options);


	if (!app) {
		return EXIT_FAILURE;
	}
	std::cout << "Test0!" << std::endl;
	print("Try running app...");
	app->run();
	std::cout << "Test1" << std::endl;

	print("Try deinitialize app...");
	app->deinit();

	print("Done.");

	return EXIT_SUCCESS;
}



// TODO: Remove this! -> Only for TESTING!
#include "multiplayer/tcpServer.h"

int main(int /*argc*/, char** /*argv*/) {
	auto& server = df::mp::TcpServer::instance();

	server.configure(45678, "127.0.0.1");
	server.onClientCallback([](int client) {
		std::string msg = "Hello from The TCP Server!\n";
		send(client, msg.c_str(), sizeof(msg.c_str()), 0);
	});

	server.start();
	server.run();

	return EXIT_SUCCESS;
}
