#pragma once

#include <registry.h>
#include <window.h>
#include <systems/audio.h>

class WorldSystem {
	public:
		static WorldSystem init(Window* window, Registry* registry, AudioSystem* audio_engine) noexcept;
		void deinit() noexcept;

		void step(const float delta) noexcept;
		void reset() noexcept;

		inline bool shouldReset() noexcept { return m_reset; }

		void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;

	private:
		static constexpr size_t MAX_EAGLES = 15;
		static constexpr size_t MAX_BUGS = 5;

		Window* m_window;
		Registry* m_registry;
		AudioSystem* m_audio_engine;

		size_t m_score;

		float m_eagle_timer;
		float m_bug_timer;

		bool m_reset;

		std::default_random_engine m_random_engine;
		std::uniform_real_distribution<float> m_uniform_distribution;
};

