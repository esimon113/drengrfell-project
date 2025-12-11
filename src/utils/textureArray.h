//
// Created by tim on 24.11.25.
//

#ifndef DRENGRFELL_TEXTUREARRAY_H
#define DRENGRFELL_TEXTUREARRAY_H

#include <common.h>
#include <assets.h>

#include "glm/gtx/io.hpp"

/*
 * Sprite: A subimage in a sprite sheet
 */

namespace df {
    class TextureArray {
    public:
        static TextureArray init(GLsizei width, GLsizei height) noexcept;
        static TextureArray init(assets::Texture asset, int widthOfSprite = 64, int heightOfSprite = 64) noexcept;
        static TextureArray init(const char* path, int widthOfSprite = 64, int heightOfSprite = 64) noexcept;

        void deinit() noexcept;
        void bind(GLuint sampler) const noexcept;
        operator int() const noexcept { return handle; }

        [[nodiscard]] std::size_t getSpriteWidth() const noexcept { return spriteWidth; }
        [[nodiscard]] std::size_t getSpriteHeight() const noexcept { return spriteHeight; }
        [[nodiscard]] std::size_t getSpritesInX() const noexcept { return spritesInX; }
        [[nodiscard]] std::size_t getSpritesInY() const noexcept { return spritesInY; }
        [[nodiscard]] std::size_t getSpriteCount() const noexcept {
            return spritesInX * spritesInY;
        }

    private:
        GLuint handle;

        std::size_t spriteWidth;
        std::size_t spriteHeight;
        std::size_t spritesInX;
        std::size_t spritesInY;
    };
}

#endif //DRENGRFELL_TEXTUREARRAY_H