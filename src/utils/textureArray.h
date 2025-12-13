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

        [[nodiscard]] int getSpriteWidth() const noexcept { return spriteWidth; }
        [[nodiscard]] int getSpriteHeight() const noexcept { return spriteHeight; }
        [[nodiscard]] int getSpritesInX() const noexcept { return spritesInX; }
        [[nodiscard]] int getSpritesInY() const noexcept { return spritesInY; }
        [[nodiscard]] int getSpriteCount() const noexcept {
            return spritesInX * spritesInY;
        }

    private:
        GLuint handle;

        int spriteWidth;
        int spriteHeight;
        int spritesInX;
        int spritesInY;
    };
}

#endif //DRENGRFELL_TEXTUREARRAY_H