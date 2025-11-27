//
// Created by tim on 24.11.25.
//

#ifndef DRENGRFELL_TEXTUREARRAY_H
#define DRENGRFELL_TEXTUREARRAY_H

#include <common.h>
#include <assets.h>

namespace df {
    class TextureArray {
    public:
        static TextureArray init(GLsizei width, GLsizei height) noexcept;
        static TextureArray init(assets::Texture asset, int tiles = -1, int tile_height = -1, int tile_width = -1) noexcept;
        static TextureArray init(const char* path, int tiles = -1, int tile_height = -1, int tile_width = -1) noexcept;

        void deinit() noexcept;
        void bind(GLuint sampler) const noexcept;
        operator int() const noexcept { return handle; }


    private:
        GLuint handle;
    };
}

#endif //DRENGRFELL_TEXTUREARRAY_H