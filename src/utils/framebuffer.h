#pragma once

#include <common.h>
#include <utils/texture.h>



namespace df {
	class Framebuffer {
		public:
			struct Descriptor {
				GLsizei width;
				GLsizei height;
				size_t colorAttachmentCount;
				bool depthAttachment;
			};

			static Framebuffer init(const Descriptor& descriptor) noexcept;
			void deinit() noexcept;

			inline void bind() const noexcept { glBindFramebuffer(GL_FRAMEBUFFER, handle); }
			inline void unbind() const noexcept { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

			inline const Texture* getColorAttachments() const noexcept { return colorAttachments; }
			inline glm::vec2 getExtent() const noexcept { return extent; }


		private:
			GLuint handle;
			glm::uvec2 extent;
			Texture* colorAttachments;
			size_t colorAttachmentCount;
			std::optional<GLuint> depthAttachment;
	};
}
