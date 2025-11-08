#include "framebuffer.h"



namespace df {
	Framebuffer Framebuffer::init(const Framebuffer::Descriptor& descriptor) noexcept {
		Framebuffer self;

		glGenFramebuffers(1, &self.handle);
		self.bind();
		self.colorAttachments = new Texture[descriptor.colorAttachmentCount];
		self.colorAttachmentCount = descriptor.colorAttachmentCount;
		std::vector<GLuint> attachments(descriptor.colorAttachmentCount);

		for (GLuint i = 0; i < descriptor.colorAttachmentCount; ++i) {
			self.colorAttachments[i] = Texture::init(descriptor.width, descriptor.height);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, self.colorAttachments[i], 0);
			attachments[i] = GL_COLOR_ATTACHMENT0 + i;
		}
		glDrawBuffers(static_cast<GLsizei>(attachments.size()), attachments.data());

		if (descriptor.depthAttachment) {
			GLuint db;
			glGenRenderbuffers(1, &db);
			self.depthAttachment = db;
			glBindRenderbuffer(GL_RENDERBUFFER, db);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, descriptor.width, descriptor.height);

			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, db);
		} else {
			self.depthAttachment = std::nullopt;
		}

		assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		self.unbind();
		self.extent = { descriptor.width, descriptor.height };

		return self;
	}


	void Framebuffer::deinit() noexcept {
		for (size_t i = 0; i < colorAttachmentCount; ++i) {
			colorAttachments[i].deinit();
		}
		delete [] colorAttachments;

		if (depthAttachment) {
			glDeleteRenderbuffers(1, &depthAttachment.value());
		}

		glDeleteFramebuffers(1, &handle);
	}
}
