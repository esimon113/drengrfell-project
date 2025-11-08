#include "mesh.h"
#include <filesystem>
#include <tiny_obj_loader.h>



namespace df {
	::std::optional<Mesh> Mesh::init(const assets::Mesh asset) noexcept {
		Mesh self;

		const ::std::string assetPath = assets::getAssetPath(asset);
		const ::std::string basePath = ::std::filesystem::path(assetPath).parent_path().string();
		tinyobj::ObjReaderConfig readerConfig;
		readerConfig.mtl_search_path = basePath;

		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(assetPath, readerConfig)) {
			fmt::println(stderr, "Failed to read mesh: {}", reader.Error());
			return ::std::nullopt;
		}

		if (!reader.Warning().empty()) {
			fmt::println(stderr, "Warning while reading mesh: {}", reader.Warning());
		}

		const auto& attrib = reader.GetAttrib();
		const auto& shapes = reader.GetShapes();

		size_t indexCount = 0;
		for (auto shape : shapes) {
			indexCount += shape.mesh.indices.size();
		}
		self.indexCount = (uint32_t)indexCount;

		const size_t index_size = sizeof(uint32_t) * indexCount;
		::std::vector<uint32_t> indices;
		indices.reserve(indexCount);

		::std::vector<ColoredVertex> vertices;
		::std::unordered_map<ColoredVertex, uint32_t> uniqueVertices;
		vertices.reserve(indexCount);

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				ColoredVertex vertex{};

				vertex.position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.color = {
					attrib.colors[3 * index.vertex_index + 0],
					attrib.colors[3 * index.vertex_index + 1],
					attrib.colors[3 * index.vertex_index + 2]
				};

				if (!uniqueVertices.contains(vertex)) {
					uniqueVertices.emplace(vertex, static_cast<uint32_t>(vertices.size()));
					vertices.emplace_back(vertex);
				}

				indices.emplace_back(uniqueVertices[vertex]);
			}
		}
		const size_t vertex_size = sizeof(ColoredVertex) * vertices.size();

		glGenVertexArrays(1, &self.vao);
		glBindVertexArray(self.vao);

		glGenBuffers(2, &self.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, self.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertex_size, vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size, indices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), (void*)(sizeof(glm::vec3)));

		return self;
	}


	void Mesh::deinit() noexcept {
		glDeleteBuffers(2, &vbo);
		glDeleteVertexArrays(1, &vao);
	}


	void Mesh::bind() const noexcept {
		glBindVertexArray(vao);
	}
}
