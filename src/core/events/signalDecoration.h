#pragma once
#include <string>
#include <assets.h>
#include "eventBus.h"

/*
 * Here, we "decorate" signals. What I mean by that is adding additional behavior on top of the "start" of signals.
 * In this case, we send an additional signal to request the playing of a sound when a signal has a sound "attached".
 * This behvior could be expended in the future, but for now, it separates the concerns of the Signal, EventBus and
 * AudioSystem classes. None of them should need to be changed, here is the central place for linking signals to sounds.
*/

namespace df::SignalDecoration {
	template<typename SignalType>
	void attachSound(SignalType& signal, const std::string& path, bool loop = false) {
		signal.connect(
			[path, loop](auto&&...) {
				EventBus::getInstance().playSoundRequested.emit(path, loop);
			}
		);
	}

	template<typename SignalType>
	void attachSound(SignalType& signal, const assets::Sound asset, bool loop = false) {
		attachSound(signal, assets::getAssetPath(asset), loop);
	}

	inline void initializeSignalDecoration() {
		EventBus& e = EventBus::getInstance();
		attachSound(e.applicationRunStarted, assets::Sound::music, true);
	}
}
