#pragma once
#include <functional>

/*
 * This idea is inspired from Signals in Godot engine and the event bus pattern.
 * In Godot, you typically create a singleton for holding all the possible events as signals,
 * and connect the classes (resp. nodes) to this singleton, in order to be able to emit signals and connect to them.
*/

namespace df {
	template<typename... Arguments>
	class Signal {
	public:
		using Callback = std::function<void(Arguments...)>;

		void connect(Callback callback) {
			callbacks.push_back(callback);
		}

		void emit(Arguments... arguments) {
			for (auto& callback : callbacks) {
				callback(arguments...);
			}
		}

	private:
		std::vector<Callback> callbacks;
	};
}
