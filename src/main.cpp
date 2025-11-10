#include <application.h>
#include <utils/commandLineOptions.h>

#include <iostream>


void print(std::string s) {
	std::cout << s << std::endl;
}


int main(int argc, char** argv) {
	print("Starting...");

	df::CommandLineOptions options = df::CommandLineOptions::parse(argc, argv);
	std::optional<df::Application> app = df::Application::init(options);

	print("Initialized app");

	if (!app) {
		return EXIT_FAILURE;
	}

	print("App exists");
	print("Try running app");

	app->run();
	print("App runs");

	print("Try deinit app");
	app->deinit();

	print("success");

	return EXIT_SUCCESS;
}
