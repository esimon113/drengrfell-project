#pragma once

#include <common.h>
#include <assets.h>
#include <miniaudio.h>



namespace df {
	class AudioSystem {
		public:
			AudioSystem() noexcept;
			~AudioSystem() noexcept;

			ma_sound* getBackgroundMusic() noexcept { return backgroundMusic; }

		private:

			ma_engine* engine;
			ma_sound* backgroundMusic;

			ma_sound* loadSound(assets::Sound asset) noexcept;

			void onPlaySoundRequested(const std::string& path);
	};
}
