#include "renderText.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <iostream>

namespace df {

    RenderTextSystem RenderTextSystem::init(Window* window, Registry* registry) noexcept {
        RenderTextSystem self;
        self.window = window;
        self.registry = registry;

        self.viewport.origin = { 0, 0 };
        self.viewport.size = window->getWindowExtent();

        self.textShader = Shader::init(assets::Shader::text).value();

        // FreeType init
        FT_Library ft;
        if (FT_Init_FreeType(&ft)) {
            std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
            return self;
        }

        std::string fontPath = getBasePath() + "/assets/fonts/static/Roboto-Regular.ttf";
        FT_Face face;
        if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
            std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
            FT_Done_FreeType(ft);
            return self;
        }

        FT_Set_Pixel_Sizes(face, 0, 48);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (unsigned char c = 0; c < 128; c++) {
            // load character glyph 
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }

            GLuint tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character ch{
                tex,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<GLuint>(face->glyph->advance.x)
            };

            self.characters.insert(std::pair<char, Character>(c, ch));
        }

        FT_Done_Face(face);
        FT_Done_FreeType(ft);

        // VAO / VBO
        glGenVertexArrays(1, &self.vao);
        glGenBuffers(1, &self.vbo);

        glBindVertexArray(self.vao);
        glBindBuffer(GL_ARRAY_BUFFER, self.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        return self;
    }


    void RenderTextSystem::renderText(
        std::string text,   // Text do display
        glm::vec2 pos,      // Position to display
        float scale,        // size
        glm::vec3 color     // color
    ) const noexcept {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glm::mat4 projection = glm::ortho(
            0.0f,
            static_cast<float>(viewport.size.x),
            0.0f,
            static_cast<float>(viewport.size.y)
        );

        textShader.use()
            .setMat4("projection", projection)
            .setVec3("textColor", color)
            .setSampler("text", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(vao);

        // iterate through all characters
        for (unsigned char c : text) {
            const Character& ch = characters.at(c);

            float xpos = pos.x + ch.bearing.x * scale;
            float ypos = pos.y - (ch.size.y - ch.bearing.y) * scale;

            float w = ch.size.x * scale;
            float h = ch.size.y * scale;
            // update VBO for each character
            float vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },

                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }
            };
            // render glyph texture over quad
            glBindTexture(GL_TEXTURE_2D, ch.textureID);
            // update content of VBO memory
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            // render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);
            // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            pos.x += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    Viewport RenderTextSystem::getViewport() const noexcept {
        return viewport;
    }

    void RenderTextSystem::step(float /*delta*/) noexcept {
        // Test Print
        /*renderText(
            "Hello Text Rendering! 01010 :: 0",
            { 20.0f, 25.0f }, // window->getWindowExtent().y - 120.0f
            0.5f,
            { 1.0f, 1.0f, 1.0f }
        );  
        */
    }

    void RenderTextSystem::reset() noexcept {

    }

    void RenderTextSystem::deinit() noexcept {
        textShader.deinit();
    }

}

