#pragma once

#include <common.h>
#include <assets.h>



namespace df {
	class Texture {
		public:
			static Texture init(const GLsizei width, const GLsizei height) noexcept;
			static Texture init(const assets::Texture asset) noexcept;
			static Texture init(const char* path) noexcept;
			static Texture createDummy() noexcept;
			void deinit() noexcept;
			void bind(const GLuint sampler) const noexcept;
			inline operator int() const noexcept { return handle; }
			GLuint getHandle() const noexcept { return handle; }

		private:
			GLuint handle;
	};
}
