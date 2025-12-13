//
// Created by tim on 24.11.25.
//

#include "textureArray.h"

#include <iostream>
#include <ostream>

namespace df {
    TextureArray TextureArray::init(const GLsizei width, const GLsizei height) noexcept {
        TextureArray self;

        glGenTextures(1, &self.handle);
        glBindTexture(GL_TEXTURE_2D_ARRAY, self);
        glTexImage3D(GL_TEXTURE_2D_ARRAY,
             0,                 // mipmap level
             GL_RGBA8,          // gpu texel format
             width,             // width
             height,            // height
             1,                 // depth
             0,                 // border
             GL_RGBA,           // cpu pixel format
             GL_UNSIGNED_BYTE,  // cpu pixel coord type
             nullptr);           // pixel data

        return self;
    }

    /**
     * Assumes a vertical stripe of quadratical tile textures
     */
    TextureArray TextureArray::init(const assets::Texture asset, const int widthOfSprite, int heightOfSprite) noexcept {
        const std::string assetPath = assets::getAssetPath(asset);
        return init(assetPath.c_str(), widthOfSprite, heightOfSprite);
    }

    /**
     * Assumes a vertical stripe of rectangular tile textures
     */
    TextureArray TextureArray::init(const char* path, int widthOfSprite, int heightOfSprite) noexcept {
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        stbi_uc* pixels = stbi_load(path, &width, &height, &channels, 4);
        if (pixels == nullptr) {
            std::cerr << "Error loading pixels from " << path << std::endl;
            return {};
        }
        if (width % widthOfSprite != 0) {
            std::cerr << "TextureArray::init: width is not divisible by widthOfSprite" << std::endl;
        }
        if (height % heightOfSprite != 0) {
            std::cerr << "TextureArray::init: height is not divisible by heightOfSprite" << std::endl;
        }

        TextureArray self;
        self.spriteWidth = widthOfSprite;
        self.spriteHeight = heightOfSprite;
        self.spritesInX = width / widthOfSprite;
        self.spritesInY = height / heightOfSprite;

        // The solution from last milestone only allows a vertical strip of textures.
        // But we want a 2D-grid for easier asset creation work.
        // The approach is to load the image as a grid and then "transform" it into a list of textures
        // (aka the format we use in the OpenGL commands)

        constexpr int bytesPerPixel = 4;
        const int spriteSize = widthOfSprite * heightOfSprite * bytesPerPixel;

        std::vector<stbi_uc> listPixels(self.getSpriteCount() * spriteSize);

        for (int spriteRow = 0; spriteRow < self.spritesInY; spriteRow++) {
            for (int spriteCol = 0; spriteCol < self.spritesInX; spriteCol++) {
                const int sprite = spriteRow * self.spritesInX + spriteCol;
                stbi_uc* destinationSprite = listPixels.data() + (sprite * spriteSize);

                for (int row = 0; row < heightOfSprite; row++) {
                    const int sourceY = (self.spritesInY - 1 - spriteRow) * heightOfSprite + row;
                    const int sourceX = spriteCol * widthOfSprite;

                    const stbi_uc* source = pixels + sourceY * (width * bytesPerPixel) + sourceX * bytesPerPixel;
                    stbi_uc* destination = destinationSprite + row * (widthOfSprite * bytesPerPixel);

                    std::memcpy(destination, source, widthOfSprite * bytesPerPixel);
                }
            }
        }

        glGenTextures(1, &self.handle);
        glBindTexture(GL_TEXTURE_2D_ARRAY, self.handle);

        glTexImage3D(GL_TEXTURE_2D_ARRAY,
             0,                 // mipmap level
             GL_RGBA8,          // gpu texel format
             widthOfSprite,     // width
             heightOfSprite,    // height
             self.getSpriteCount(),           // depth
             0,                 // border
             GL_RGBA,           // cpu pixel format
             GL_UNSIGNED_BYTE,  // cpu pixel coord type
             listPixels.data());           // pixel data

        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 4);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

        stbi_image_free(pixels);

        return self;
    }


    void TextureArray::deinit() noexcept {
        glDeleteTextures(1, &handle);
    }


    void TextureArray::bind(const GLuint sampler) const noexcept {
        glActiveTexture(GL_TEXTURE0 + sampler);
        glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
    }
}
