#pragma once

#include <common.h>
#include <cstdint>
#include <string>



namespace df {
	class CommandLineOptions {
	  public:
		enum struct Flags : size_t {
			HELP = 0,
			X11,
			count
		};


		struct Flag {
			std::string_view longName;
			std::optional<std::string_view> shortName;
			std::string_view help;

			bool match(const char* flag) const noexcept {
				return !longName.compare(flag) || (shortName.has_value() && !shortName->compare(flag));
			}
		};


		static constexpr std::array<Flag, static_cast<size_t>(Flags::count)> FLAGS = {
			Flag{"--help", "-h", "Show this message."},
			Flag{"--X11", std::nullopt, "Force the game to use X11 for windowing. Only available on Linux."},
		};


		static CommandLineOptions parse(const size_t argc, char* const* argv) noexcept {
			CommandLineOptions options{};

			for (size_t i = 1; i < argc; ++i) {
				const std::string_view arg{argv[i]};
				if ((arg == "--host" || arg == "--port" || arg == "--name") && i + 1 >= argc) {
					fmt::println(stderr, "The \"{}\" option needs a value. See --help.", arg);
					continue;
				}
				if (arg == "--host") {
					options.host = argv[++i];
					continue;
				}
				if (arg == "--port") {
					const std::string_view text{argv[++i]};
					unsigned value = 0;
					bool ok = !text.empty();
					for (const char digit : text) {
						if (digit < '0' || digit > '9') {
							ok = false;
							break;
						}
						value = value * 10u + static_cast<unsigned>(digit - '0');
						if (value > 65535u) {
							ok = false;
							break;
						}
					}
					if (!ok || value == 0) {
						fmt::println(stderr, "Invalid port \"{}\". See --help.", text);
					} else {
						options.port = static_cast<uint16_t>(value);
					}
					continue;
				}
				if (arg == "--name") {
					options.playerName = argv[++i];
					continue;
				}

				for (size_t j = 0; j < static_cast<size_t>(Flags::count); ++j) {
					if (!FLAGS[j].match(argv[i]))
						continue;

					Flags flag = static_cast<Flags>(j);
					switch (flag) {
					case Flags::HELP:
						fmt::println(stderr, "usage: {} [options]\nOptions:", argv[0]);
						for (const Flag& f : FLAGS) {
							if (f.shortName)
								fmt::println(stderr, "\t{},\t{}\t{}", f.longName, f.shortName.value(), f.help);
							else
								fmt::println(stderr, "\t{}\t\t{}", f.longName, f.help);
						}
						fmt::println(stderr, "\t--host <address>\tServer address. Default 127.0.0.1.");
						fmt::println(stderr, "\t--port <port>\t\tServer port. Default 7777.");
						fmt::println(stderr, "\t--name <name>\t\tPlayer name. Default Player.");
						options.help = true;
						break;

#if defined(__linux__)
					case Flags::X11:
						options.x11 = true;
						break;
#endif

					case Flags::count:
					default:
						if (FLAGS[j].shortName)
							fmt::println(stderr, "The \"{}\"(\"{}\") flag is not supported in this build. See --help.", FLAGS[j].longName, FLAGS[j].shortName.value());
						else
							fmt::println(stderr, "The \"{}\" flag is not supported in this build. See --help.", FLAGS[j].longName);
						break;
					}
				}
			}

			return options;
		}


		inline bool hasHelp() const noexcept { return help; }
		inline bool hasX11() const noexcept { return x11; }
		inline const std::string& getHost() const noexcept { return host; }
		inline uint16_t getPort() const noexcept { return port; }
		inline const std::string& getPlayerName() const noexcept { return playerName; }


	  private:
		bool help = false;
		bool x11 = false;
		std::string host{"127.0.0.1"};
		uint16_t port{7777};
		std::string playerName{"Player"};
	};
} // namespace df
