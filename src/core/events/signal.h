#pragma once
#include <functional>
#include <string>

/*
 * This idea is inspired from Signals in Godot engine and the event bus pattern.
 * In Godot, you typically create a singleton for holding all the possible events as signals,
 * and connect the classes (resp. nodes) to this singleton, in order to be able to emit signals and connect to them.
*/

namespace df {
	template<typename... Arguments>
	class Signal {
	public:
		Signal() = default;
		explicit Signal(std::string signalName) : name(std::move(signalName)) {}
		~Signal() = default;

		using Callback = std::function<void(Arguments...)>;

		void connect(Callback callback, const std::string& identifier) {
			fmt::println("Connected callback {} to signal {}", identifier, name);
			auto [iterator, inserted] = callbacks.emplace(identifier, callback);
			if (!inserted) {
				fmt::print("Callback {} is already connected to signal {}\n", identifier, name);
			}
		}

		void disconnect(const std::string& identifier) {
			fmt::println("Disconnected callback {} from signal {}", identifier, name);
			callbacks.erase(identifier);
		}

		template<typename... Arguments2>
		void emit(Arguments2&&... arguments) {
			fmt::println("Emitted signal {}", name);
			const auto currentCallbacks = callbacks;
			for (auto const& [identifier, callback] : currentCallbacks) {
				callback(std::forward<Arguments>(arguments)...);
			}
		}

		std::string name{};
	private:
		std::unordered_map<std::string, Callback> callbacks{};
	};
}
