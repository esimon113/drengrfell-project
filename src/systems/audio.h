#pragma once

#include <common.h>
#include <assets.h>
#include <miniaudio.h>

class AudioSystem {
	public:
		AudioSystem() = default;
		~AudioSystem() = default;

		static AudioSystem init() noexcept;
		void deinit() noexcept;

		inline ma_sound* eatSound() noexcept { return m_eat_sound; }
		inline ma_sound* deadSound() noexcept { return m_dead_sound; }
		inline ma_sound* backgroundMusic() noexcept { return m_background_music; }

	private:
		ma_engine* m_engine;
		ma_sound* m_eat_sound;
		ma_sound* m_dead_sound;
		ma_sound* m_background_music;

		ma_sound* loadSound(const assets::Sound asset) noexcept;
};
