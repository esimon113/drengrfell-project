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
				std::string_view m_long;
				std::optional<std::string_view> m_short;
				std::string_view m_help;

				bool match(const char* flag) const noexcept {
					return !m_long.compare(flag) || (m_short.has_value() && !m_short->compare(flag));
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
									if (f.m_short)
										fmt::println(stderr, "\t{},\t{}\t{}", f.m_long, f.m_short.value(), f.m_help);
									else
										fmt::println(stderr, "\t{}\t\t{}", f.m_long, f.m_help);
								}
								options.m_help = true;
								break;
	#if defined(__linux__)
							case Flags::X11:
								options.m_x11 = true;
								break;
	#endif
							case Flags::count:
							default:
								if (FLAGS[j].m_short)
									fmt::println(stderr, "The \"{}\"(\"{}\") flag is not supported in this build. See --help.", FLAGS[j].m_long, FLAGS[j].m_short.value());
								else
									fmt::println(stderr, "The \"{}\" flag is not supported in this build. See --help.", FLAGS[j].m_long);
								break;
						}
					}
				}

				return options;
			}

			inline bool help() const noexcept { return m_help; }
			inline bool x11() const noexcept { return m_x11; }

		private:
			bool m_help = false;
			bool m_x11 = false;
	};
}