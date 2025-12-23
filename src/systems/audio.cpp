#include "audio.h"

#include <iostream>
#include <ostream>

#include "events/eventBus.h"


namespace df {
	static bool constructed = false;

	AudioSystem::AudioSystem() noexcept {
		if (constructed) {
			std::cerr << "WARNING: Audio system already constructed!" << std::endl;
		}
		constructed = true;
		this->engine = new ma_engine;
		ma_engine_init(nullptr, this->engine);
		this->backgroundMusic = this->loadSound(assets::Sound::music);

		EventBus::getInstance().playSoundRequested.connect(
			[this](const std::string& path) {
				this->onPlaySoundRequested(path);
			}
		);
	}


	AudioSystem::~AudioSystem() noexcept {
		ma_sound_uninit(backgroundMusic);
		delete backgroundMusic;

		ma_engine_uninit(engine);
		delete engine;
	}


	ma_sound* AudioSystem::loadSound(const assets::Sound asset) noexcept {
		const std::string assetPath = assets::getAssetPath(asset);
		ma_sound* sound = nullptr;

		if ((sound = (new ma_sound)) == nullptr) {
			fmt::println(stderr, "Failed to allocate sound");

			return nullptr;
		}

		ma_result result;
		if ((result = ma_sound_init_from_file(engine, assetPath.c_str(), 0, nullptr, nullptr, sound)) != MA_SUCCESS) {
			fmt::println(stderr, "Failed to load \"{}\": {}", assetPath, ma_result_description(result));
			delete sound;

			return nullptr;
		}

		return sound;
	}


	void AudioSystem::onPlaySoundRequested(const std::string& path) {
		fmt::print("Playing sound requested: {}\n", path);

		ma_sound* music = this->getBackgroundMusic();
		ma_sound_set_looping(music, MA_FALSE);
		ma_sound_start(music);
	}
}
