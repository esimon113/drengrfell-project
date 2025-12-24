#include "audio.h"

#include <iostream>
#include <ostream>

#include "events/eventBus.h"


namespace df {
	AudioSystem::Sound::Sound(ma_engine* pEngine, const std::string &path) noexcept {
		ma_sound* newSound = nullptr;

		if ((newSound = (new ma_sound)) == nullptr) {
			fmt::println(stderr, "Failed to allocate sound");
			newSound = nullptr;
		}

		ma_result result;
		if ((result = ma_sound_init_from_file(pEngine, path.c_str(), 0, nullptr, nullptr, newSound)) != MA_SUCCESS) {
			fmt::println(stderr, "Failed to load \"{}\": {}", path, ma_result_description(result));
			delete newSound;
			newSound = nullptr;
		}

		sound.reset(newSound);
	}


	static bool constructed = false;
	AudioSystem::AudioSystem() noexcept {
		if (constructed) {
			std::cerr << "WARNING: Audio system already constructed!" << std::endl;
		}
		constructed = true;
		this->engine.reset(new ma_engine);
		ma_engine_init(nullptr, this->engine.get());

		EventBus::getInstance().playSoundRequested.connect(
			[this](const std::string& path, bool loop) {
				this->onPlaySoundRequested(path, loop);
			}
		);
	}


	bool AudioSystem::loadSound(const std::string& path) {
		return sounds.emplace(path, std::make_unique<Sound>(engine.get(), path)).second;
	}


	void AudioSystem::onPlaySoundRequested(const std::string& path, const bool loop) {
		fmt::print("Playing sound requested: {}\n", path);

		if (!this->sounds.contains(path)) {
			if (!loadSound(path)) {
				return;
			}
		}

		if (this->sounds.contains(path)) {
			const Sound* sound = this->sounds[path].get();
			ma_sound* music = sound->get();
			ma_sound_set_looping(music, loop ? MA_TRUE : MA_FALSE);
			ma_sound_start(music);
		}
	}
}
