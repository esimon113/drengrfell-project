#include "audio.h"



namespace df {
	AudioSystem AudioSystem::init() noexcept {
		AudioSystem self;

		self.engine = new ma_engine;
		ma_engine_init(nullptr, self.engine);
		self.backgroundMusic = self.loadSound(assets::Sound::music);

		return self;
	}


	void AudioSystem::deinit() noexcept {
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
}
