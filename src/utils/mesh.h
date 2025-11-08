#pragma once

#include <common.h>

#include "assets.h"



namespace df {
	struct ColoredVertex {
		glm::vec3 position;
		glm::vec3 color;

		bool operator==(const ColoredVertex& other) const {
			return position == other.position && color == other.color;
		}
	};


	class Mesh {
		public:
			static ::std::optional<Mesh> init(const assets::Mesh asset) noexcept;
			void deinit() noexcept;
			void bind() const noexcept;
			inline uint32_t getIndexCount() const noexcept { return indexCount; };

		private:
			uint32_t indexCount;
			GLuint vao;
			GLuint vbo;
			GLuint ebo;
	};
}


namespace std {
	template<> struct hash<df::ColoredVertex> {
		std::size_t operator() (df::ColoredVertex const &vertex) const {
			return ((std::hash<glm::vec3>() (vertex.position) ^ (std::hash<glm::vec3>() (vertex.color) << 1)) >> 1);
		}
	};
}
