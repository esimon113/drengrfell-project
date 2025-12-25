#include "audio.h"

#include <iostream>
#include <ostream>
#include <utility>

#include "events/eventBus.h"


namespace df {
	AudioSystem::Sound::Sound(ma_engine* pEngine, const std::string& path) {
		ma_sound* newSound = new ma_sound();
		if (ma_result result; (result = ma_sound_init_from_file(pEngine, path.c_str(), 0, nullptr, nullptr, newSound)) != MA_SUCCESS) {
			fmt::println(stderr, "Failed to load \"{}\": {}", path, ma_result_description(result));
			delete newSound;
			newSound = nullptr;
		}

		sound.reset(newSound);
	}


	AudioSystem::AudioSystem(std::shared_ptr<EventBus> bus) : eventBus(bus) {
		this->engine.reset(new ma_engine);
		if (ma_result result; (result = ma_engine_init(nullptr, this->engine.get())) != MA_SUCCESS) {
			fmt::println(stderr, "Failed to initialize ma_engine: {}", ma_result_description(result));
		}

		eventBus->playSoundRequested.connect(
			[this](const std::string& path, const bool loop) {
				this->onPlaySoundRequested(path, loop);
			},
			"AudioSystem::onPlaySoundRequested"
		);
	}

	AudioSystem::~AudioSystem() noexcept {
		eventBus->playSoundRequested.disconnect("AudioSystem::onPlaySoundRequested");
	}


	bool AudioSystem::loadSound(const std::string& path) {
		auto s = std::make_unique<Sound>(engine.get(), path);
		if (s->get() == nullptr) {
			return false;
		}
		return sounds.emplace(path, std::move(s)).second;
	}


	bool AudioSystem::isSoundLoaded(const std::string& path) const {
		return sounds.contains(path);
	}


	void AudioSystem::onPlaySoundRequested(const std::string& path, const bool loop) {
		fmt::println("Playing sound requested: {}\n", path);

		if (!isSoundLoaded(path)) {
			if (!loadSound(path)) {
				return;
			}
		}
		ma_sound* music = sounds.at(path)->get();
		if (!music) return;
		ma_sound_set_looping(music, loop ? MA_TRUE : MA_FALSE);
		ma_sound_start(music);
	}
}
