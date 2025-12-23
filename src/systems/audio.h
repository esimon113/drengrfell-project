#pragma once

#include <common.h>
#include <assets.h>
#include <miniaudio.h>



namespace df {
	class AudioSystem {
		public:
			AudioSystem() = default;
			~AudioSystem() = default;

			static AudioSystem init() noexcept;
			void deinit() noexcept;

			ma_sound* getBackgroundMusic() noexcept { return backgroundMusic; }


		private:
			ma_engine* engine;
			ma_sound* backgroundMusic;

			ma_sound* loadSound(assets::Sound asset) noexcept;
	};
}
