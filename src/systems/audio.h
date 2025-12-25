#pragma once

#include <common.h>
#include <memory>
#include <miniaudio.h>


namespace df {
	class EventBus;
	class AudioSystem {
		public:
			explicit AudioSystem(std::shared_ptr<EventBus> bus);
			~AudioSystem() noexcept;
		private:
			class Sound {
			public:
				Sound(ma_engine* pEngine, const std::string &path);
				[[nodiscard]] ma_sound* get() const noexcept { return sound.get(); }
			private:
				struct Destructor {
					void operator()(ma_sound* s) const noexcept {
						if (s) {
							ma_sound_uninit(s);
							delete s;
						}
					}
				};
				std::unique_ptr<ma_sound, Destructor> sound;
			};

			struct EngineDestructor {
				void operator()(ma_engine* e) const noexcept {
					if (e) {
						ma_engine_uninit(e);
						delete e;
					}
				}
			};

			std::shared_ptr<EventBus> eventBus;
			std::unique_ptr<ma_engine, EngineDestructor> engine;
			std::unordered_map<std::string, std::unique_ptr<Sound>> sounds;

			bool loadSound(const std::string& path);
			bool isSoundLoaded(const std::string& path) const;
			void onPlaySoundRequested(const std::string& path, bool loop = false);
	};
}
