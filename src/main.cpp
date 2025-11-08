#include <application.h>
#include <utils/commandLineOptions.h>



int main(int argc, char** argv) {
	df::CommandLineOptions options = df::CommandLineOptions::parse(argc, argv);
	std::optional<df::Application> app = df::Application::init(options);

	if (!app) {
		return EXIT_FAILURE;
	}

	app->run();
	app->deinit();

	return EXIT_SUCCESS;
}
