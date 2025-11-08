#pragma once

#include <common.h>



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
				Flag{ "--help", "-h", "Show this message." },
				Flag{ "--X11", std::nullopt, "Force the game to use X11 for windowing. Only available on Linux." },
			};


			static CommandLineOptions parse(const size_t argc, char* const * argv) noexcept {
				CommandLineOptions options{};

				for (size_t i = 1; i < argc; ++i) {
					for (size_t j = 0; j < static_cast<size_t>(Flags::count); ++j) {
						if (!FLAGS[j].match(argv[i])) continue;

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


		private:
			bool help = false;
			bool x11 = false;
	};
}
