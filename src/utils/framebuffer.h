#pragma once

#include <common.h>
#include <utils/texture.h>

class Framebuffer {
	public:
		struct Descriptor {
			GLsizei width;
			GLsizei height;
			size_t color_attachment_count;
			bool depth_attachment;
		};

		static Framebuffer init(const Descriptor& descriptor) noexcept;
		void deinit() noexcept;

		inline void bind() const noexcept { glBindFramebuffer(GL_FRAMEBUFFER, m_handle); }
		inline void unbind() const noexcept { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

		inline const Texture* colorAttachments() const noexcept { return m_color_attachments; }
		inline glm::vec2 extent() const noexcept { return m_extent; }

	private:
		GLuint m_handle;
		glm::uvec2 m_extent;
		Texture* m_color_attachments;
		size_t m_color_attachment_count;
		std::optional<GLuint> m_depth_attachment;
};
