#include "framebuffer.h"



namespace df {
	Framebuffer Framebuffer::init(const Framebuffer::Descriptor& descriptor) noexcept {
		Framebuffer self;
		
		glGenFramebuffers(1, &self.m_handle);
		self.bind();
		self.m_color_attachments = new Texture[descriptor.color_attachment_count];
		self.m_color_attachment_count = descriptor.color_attachment_count;
		std::vector<GLuint> attachments(descriptor.color_attachment_count);
		for (GLuint i = 0; i < descriptor.color_attachment_count; ++i) {
			self.m_color_attachments[i] = Texture::init(descriptor.width, descriptor.height);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, self.m_color_attachments[i], 0);
			attachments[i] = GL_COLOR_ATTACHMENT0 + i;
		}
		glDrawBuffers(static_cast<GLsizei>(attachments.size()), attachments.data());

		if (descriptor.depth_attachment) {
			GLuint db;
			glGenRenderbuffers(1, &db);
			self.m_depth_attachment = db;
			glBindRenderbuffer(GL_RENDERBUFFER, db);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, descriptor.width, descriptor.height);

			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, db);
		} else {
			self.m_depth_attachment = std::nullopt;
		}

		assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		self.unbind();
		self.m_extent = { descriptor.width, descriptor.height };

		return self;
	}

	void Framebuffer::deinit() noexcept {
		for (size_t i = 0; i < m_color_attachment_count; ++i) {
			m_color_attachments[i].deinit();
		}
		delete [] m_color_attachments;
		
		if (m_depth_attachment) {
			glDeleteRenderbuffers(1, &m_depth_attachment.value());
		}

		glDeleteFramebuffers(1, &m_handle);
	}
}