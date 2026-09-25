#include "multiplayer/asgard.h"
#include "multiplayer/bifrost.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
	uint16_t port = df::bifrost::DEFAULT_PORT;
	if (argc >= 2) {
		try {
			const int parsed = std::stoi(argv[1]);
			if (parsed > 0 && parsed <= 65535) {
				port = static_cast<uint16_t>(parsed);
			}
		} catch (const std::exception&) {
			std::cerr << "Usage: drengrfell_server [port]\n";
			return EXIT_FAILURE;
		}
	}

	df::bifrost::Asgard server;
	server.configure(port);
	std::cout << "drengrfell_server listening on port " << port << std::endl;
	server.run();
	return EXIT_SUCCESS;
}
