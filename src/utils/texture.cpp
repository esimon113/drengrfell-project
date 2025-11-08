#include "texture.h"



namespace df {
	Texture Texture::init(const GLsizei width, const GLsizei height) noexcept {
		Texture self;

		glGenTextures(1, &self.handle);
		glBindTexture(GL_TEXTURE_2D, self);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		return self;
	}


	Texture Texture::init(const assets::Texture asset) noexcept {
		const std::string assetPath = assets::getAssetPath(asset);
		return init(assetPath.c_str());
	}


	Texture Texture::init(const char* path) noexcept {
		int w, h, c; // assuming these stand for: width, height, channels ?!
		stbi_set_flip_vertically_on_load(true);
		stbi_uc* data = stbi_load(path, &w, &h, &c, 4);

		Texture self;
		glGenTextures(1, &self.handle);
		glBindTexture(GL_TEXTURE_2D, self.handle);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(data);

		return self;
	}


	void Texture::deinit() noexcept {
		glDeleteTextures(1, &handle);
	}


	void Texture::bind(const GLuint sampler) const noexcept {
		glActiveTexture(GL_TEXTURE0 + sampler);
		glBindTexture(GL_TEXTURE_2D, handle);
	}
}
