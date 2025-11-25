#include <application.h>
#include <utils/commandLineOptions.h>

#include <iostream>


void print(std::string s) {
	std::cout << s << std::endl;
}


int main(int argc, char** argv) {
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
