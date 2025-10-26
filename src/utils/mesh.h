#pragma once

#include <common.h>
#include "assets.h"

struct ColoredVertex {
	glm::vec3 m_position;
	glm::vec3 m_color;

	bool operator==(const ColoredVertex& other) const {
		return m_position == other.m_position && m_color == other.m_color;
	}
};

namespace std
{
    template<> struct hash<ColoredVertex>
    {
        std::size_t operator() (ColoredVertex const &vertex) const
        {
            return ((std::hash<glm::vec3>() (vertex.m_position) ^
                    (std::hash<glm::vec3>() (vertex.m_color) << 1)) >> 1);
        }
    };
}

class Mesh {
	public:
		static std::optional<Mesh> init(const assets::Mesh asset) noexcept;
		void deinit() noexcept;
		void bind() const noexcept;
		inline uint32_t indexCount() const noexcept { return m_index_count; };

	private:
		uint32_t m_index_count;
		GLuint m_vao;
		GLuint m_vbo;
		GLuint m_ebo;
};
