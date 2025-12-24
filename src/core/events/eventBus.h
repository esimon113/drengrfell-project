#pragma once
#include <string>

#include "./events/signal.h"

/*
 * This idea is inspired from Signals in Godot engine and the event bus pattern.
 * In Godot, you typically create a singleton for holding all the possible events as signals,
 * and connect the classes (resp. nodes) to this singleton, in order to be able to emit signals and connect to them.
 *
 * Naming convention for signals:
 *     Like member variables, but as past participle (-ed, but also written, eaten)
 *
 * Naming convention for signal handling callbacks:
 *     on + Signal
*/

namespace df {
	struct EventBus {
		static EventBus& getInstance() {
			static EventBus instance;
			return instance;
		}

		// Application Events
		Signal<> applicationRunStarted;

		// Game Events
		Signal<int> tilePicked;

		// Technical

		// Do not emit playSoundRequested explicitly !
		// Instead, use the signal decoration feature
		// and add a new sound attachment to a signal
		// from above in df::SignalDecoration::initializeSignalDecoration
		Signal<const std::string&, bool> playSoundRequested;
	};
}
