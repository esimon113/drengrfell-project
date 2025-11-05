#include "mesh.h"
#include <filesystem>
#include <tiny_obj_loader.h>



namespace df {
	std::optional<Mesh> Mesh::init(const assets::Mesh asset) noexcept {
		Mesh self;

		const std::string asset_path = assets::getAssetPath(asset);
		const std::string base_path = std::filesystem::path(asset_path).parent_path().string();
		tinyobj::ObjReaderConfig reader_config;
		reader_config.mtl_search_path = base_path;

		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(asset_path, reader_config)) {
			fmt::println(stderr, "Failed to read mesh: {}", reader.Error());
			return std::nullopt;
		}

		if (!reader.Warning().empty()) {
			fmt::println(stderr, "Warning while reading mesh: {}", reader.Warning());
		}

		const auto& attrib = reader.GetAttrib();
		const auto& shapes = reader.GetShapes();

		size_t index_count = 0;
		for (auto shape : shapes) {
			index_count += shape.mesh.indices.size();
		}
		self.m_index_count = (uint32_t)index_count;

		const size_t index_size = sizeof(uint32_t) * index_count;
		std::vector<uint32_t> indices;
		indices.reserve(index_count);

		std::vector<ColoredVertex> vertices;
		std::unordered_map<ColoredVertex, uint32_t> unique_vertices;
		vertices.reserve(index_count);

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				ColoredVertex vertex{};
				vertex.m_position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};
				vertex.m_color = {
					attrib.colors[3 * index.vertex_index + 0],
					attrib.colors[3 * index.vertex_index + 1],
					attrib.colors[3 * index.vertex_index + 2]
				};

				if (!unique_vertices.contains(vertex)) {
					unique_vertices.emplace(vertex, static_cast<uint32_t>(vertices.size()));
					vertices.emplace_back(vertex);
				}

				indices.emplace_back(unique_vertices[vertex]);
			}
		}
		const size_t vertex_size = sizeof(ColoredVertex) * vertices.size();

		glGenVertexArrays(1, &self.m_vao);
		glBindVertexArray(self.m_vao);

		glGenBuffers(2, &self.m_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, self.m_vbo);
		glBufferData(GL_ARRAY_BUFFER, vertex_size, vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self.m_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size, indices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), (void*)(sizeof(glm::vec3)));

		return self;
	}


	void Mesh::deinit() noexcept {
		glDeleteBuffers(2, &m_vbo);
		glDeleteVertexArrays(1, &m_vao);
	}


	void Mesh::bind() const noexcept {
		glBindVertexArray(m_vao);
	}
}
