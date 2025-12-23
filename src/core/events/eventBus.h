#pragma once
#include <string>

#include "./events/signal.h"

/*
 * This idea is inspired from Signals in Godot engine and the event bus pattern.
 * In Godot, you typically create a singleton for holding all the possible events as signals,
 * and connect the classes (resp. nodes) to this singleton, in order to be able to emit signals and connect to them.
*/

namespace df {
	struct EventBus {
		static EventBus& getInstance() {
			static EventBus instance;
			return instance;
		}

		Signal<std::string> somethingHappened;
	};
}
