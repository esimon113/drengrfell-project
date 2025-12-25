#pragma once
#include <string>

#include "assets.h"
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

#define RegisterSignal(name, ...) Signal<__VA_ARGS__> name{#name}

namespace df {
	class EventBus {
	public:
		EventBus() {
			initializeSignalDecoration();
		}
		~EventBus() = default;

		// Application Events {
		RegisterSignal(applicationRunStarted);
		// }

		// In-game Events {
		// RegisterSignal(tilePicked, int);
		// }

		// Technical Events {
		// Do not emit playSoundRequested explicitly !
		// Instead, use the signal decoration feature
		// and add a new sound attachment to a signal
		// from above in df::SignalDecoration::initializeSignalDecoration
		RegisterSignal(playSoundRequested, const std::string&, const bool);
		// }

	private:
		// Signal Decoration {
		template<typename SignalType>
		void attachSound(SignalType& signal, const std::string& path, const bool loop = false) {
			signal.connect(
				[this, path, loop](auto&&...) {
					this->playSoundRequested.emit(path, std::move(loop));
				},
				"EventBus::attachSound"
			);
		}

		template<typename SignalType>
		void attachSound(SignalType& signal, const assets::Sound asset, bool loop = false) {
			attachSound(signal, assets::getAssetPath(asset), loop);
		}

		inline void initializeSignalDecoration() {
			attachSound(this->applicationRunStarted, assets::Sound::music, true);
		}

		// }
	};
}
