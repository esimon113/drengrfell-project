#include "fmt/base.h"
// #include <application.h>
#include <cstdlib>
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
#include "multiplayer/tcpServer.h"

int main(int /*argc*/, char** /*argv*/) {
	auto& server = df::mp::TcpServer::instance();
	fmt::println("TcpServer instance created!");

	server.configure(45678, "127.0.0.1");
	fmt::println("TcpServer configured!");
	server.onClientCallback([](int client) {
		std::string msg = "Hello from The TCP Server!\n";
		send(client, msg.c_str(), sizeof(msg.c_str()), 0);
	});
	fmt::println("Client callback set");

	server.start();
	fmt::println("TcpServer started");
	server.run();
	fmt::println("TcpServer runs");

	return EXIT_SUCCESS;
}
