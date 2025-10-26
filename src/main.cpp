#include <application.h>
#include <utils/command_line_options.h>

int main(int argc, char** argv) {
	CommandLineOptions options = CommandLineOptions::parse(argc, argv);
	std::optional<Application> app = Application::init(options);
	if (!app) {
		return EXIT_FAILURE;
	}
	app->run();
	app->deinit();
	return EXIT_SUCCESS;
}
