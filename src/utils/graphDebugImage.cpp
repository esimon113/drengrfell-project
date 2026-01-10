#include "graphDebugImage.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <glm/glm.hpp>

#include "graph.h"
#include "worldNodeMapper.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
// Disable warnings for third-party stb library
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <stb_image_write.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif


namespace df::utils {
	namespace {
		std::filesystem::path expandPath(const std::filesystem::path& path) {
			std::string pathStr = path.string();
			if (pathStr.empty() || pathStr[0] != '~') {
				return path;
			}

			const char* home = std::getenv("HOME");
			if (!home) {
				throw std::runtime_error("HOME environment variable not set");
			}

			return std::filesystem::path(home) / pathStr.substr(2); // Skip "~/"
		}

		struct Color {
			uint8_t r, g, b, a;
		};

		template <typename Handle>
		bool isValidHandle(Handle handle) {
			return handle != nullptr && handle->getId() != SIZE_MAX;
		}

		void setPixel(std::vector<uint8_t>& img, int width, int height, int x, int y, Color color) {
			if (x < 0 || y < 0 || x >= width || y >= height)
				return;
			const size_t idx = static_cast<size_t>((y * width + x) * 4);
			img[idx + 0] = color.r;
			img[idx + 1] = color.g;
			img[idx + 2] = color.b;
			img[idx + 3] = color.a;
		}

		void drawLine(std::vector<uint8_t>& img, int width, int height, glm::ivec2 a, glm::ivec2 b, Color color) {
			int x0 = a.x;
			int y0 = a.y;
			int x1 = b.x;
			int y1 = b.y;

			int dx = std::abs(x1 - x0);
			int sx = x0 < x1 ? 1 : -1;
			int dy = -std::abs(y1 - y0);
			int sy = y0 < y1 ? 1 : -1;
			int err = dx + dy;

			while (true) {
				setPixel(img, width, height, x0, y0, color);
				if (x0 == x1 && y0 == y1)
					break;
				int e2 = 2 * err;
				if (e2 >= dy) {
					err += dy;
					x0 += sx;
				}
				if (e2 <= dx) {
					err += dx;
					y0 += sy;
				}
			}
		}

		void drawCircleFilled(std::vector<uint8_t>& img, int width, int height, glm::ivec2 center, int radius, Color color) {
			for (int y = -radius; y <= radius; ++y) {
				for (int x = -radius; x <= radius; ++x) {
					if (x * x + y * y <= radius * radius)
						setPixel(img, width, height, center.x + x, center.y + y, color);
				}
			}
		}

		constexpr int kGlyphWidth = 3;
		constexpr int kGlyphHeight = 5;
		using Glyph = std::array<uint8_t, kGlyphHeight>;

		constexpr std::array<Glyph, 10> kDigitGlyphs = {{
			{0b111, 0b101, 0b101, 0b101, 0b111}, // 0
			{0b010, 0b110, 0b010, 0b010, 0b111}, // 1
			{0b111, 0b001, 0b111, 0b100, 0b111}, // 2
			{0b111, 0b001, 0b111, 0b001, 0b111}, // 3
			{0b101, 0b101, 0b111, 0b001, 0b001}, // 4
			{0b111, 0b100, 0b111, 0b001, 0b111}, // 5
			{0b111, 0b100, 0b111, 0b101, 0b111}, // 6
			{0b111, 0b001, 0b001, 0b001, 0b001}, // 7
			{0b111, 0b101, 0b111, 0b101, 0b111}, // 8
			{0b111, 0b101, 0b111, 0b001, 0b111}	// 9
		}};

		constexpr Glyph kMinusGlyph = {0b000, 0b000, 0b111, 0b000, 0b000};

		void drawGlyph(std::vector<uint8_t>& img, int width, int height, int x, int y, char c, Color color) {
			const Glyph* glyph = nullptr;
			if (c >= '0' && c <= '9') {
				glyph = &kDigitGlyphs[static_cast<size_t>(c - '0')];
			} else if (c == '-') {
				glyph = &kMinusGlyph;
			} else {
				return;
			}

			for (int row = 0; row < kGlyphHeight; ++row) {
				uint8_t bits = (*glyph)[row];
				for (int col = 0; col < kGlyphWidth; ++col) {
					if (bits & (1 << (kGlyphWidth - 1 - col)))
						setPixel(img, width, height, x + col, y + row, color);
				}
			}
		}

		void drawText(std::vector<uint8_t>& img, int width, int height, int x, int y, const std::string& text, Color color) {
			int cursorX = x;
			for (char c : text) {
				drawGlyph(img, width, height, cursorX, y, c, color);
				cursorX += kGlyphWidth + 1;
			}
		}

		std::vector<size_t> gatherTileIds(const Graph& graph) {
			std::vector<size_t> ids;
			ids.reserve(graph.getTiles().size());
			for (const auto& tile : graph.getTiles())
				if (tile)
					ids.push_back(tile->getId());
			std::sort(ids.begin(), ids.end());
			ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
			return ids;
		}

		std::vector<size_t> gatherEdgeIds(const Graph& graph) {
			std::vector<size_t> ids;
			ids.reserve(graph.getEdges().size());
			for (const auto& edge : graph.getEdges())
				if (edge)
					ids.push_back(edge->getId());
			std::sort(ids.begin(), ids.end());
			ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
			return ids;
		}

		std::vector<size_t> gatherVertexIds(const Graph& graph) {
			std::vector<size_t> ids;
			ids.reserve(graph.getVertices().size());
			for (const auto& vertex : graph.getVertices())
				if (vertex)
					ids.push_back(vertex->getId());
			std::sort(ids.begin(), ids.end());
			ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
			return ids;
		}
	} // namespace


	void writeGraphDebugImage(const Graph& graph, const std::filesystem::path& outputPath) {
		fmt::println("[DEBUG graphDebugImage] Graph has {} tiles, {} edges, {} vertices",
			graph.getTiles().size(), graph.getEdges().size(), graph.getVertices().size());
			
		if (graph.getMapWidth() == 0)
			throw std::runtime_error("Cannot render graph debug image: map width is zero.");

		const float hexRadiusPx = 40.0f;
		const float marginPx = 32.0f;
		const auto vertexOffsets = WorldNodeMapper::getVertexOffsets(1.0f);

		// First pass: determine bounding box in pixel space
		float minX = (std::numeric_limits<float>::max)();
		float minY = (std::numeric_limits<float>::max)();
		float maxX = -(std::numeric_limits<float>::max)();
		float maxY = -(std::numeric_limits<float>::max)();

		const auto tileIds = gatherTileIds(graph);
		for (size_t tileId : tileIds) {
			const uint32_t row = static_cast<uint32_t>(tileId / graph.getMapWidth());
			const uint32_t col = static_cast<uint32_t>(tileId % graph.getMapWidth());
			glm::vec2 center = WorldNodeMapper::getTilePosition(row, col) * hexRadiusPx;

			for (const auto& offset : vertexOffsets) {
				glm::vec2 p = center + offset * hexRadiusPx;
				minX = std::min(minX, p.x);
				minY = std::min(minY, p.y);
				maxX = std::max(maxX, p.x);
				maxY = std::max(maxY, p.y);
			}
		}

		if (!std::isfinite(minX) || !std::isfinite(maxX))
			throw std::runtime_error("Cannot render graph debug image: no tiles found.");

		const int widthPx = static_cast<int>(std::ceil(maxX - minX + marginPx * 2.0f));
		const int heightPx = static_cast<int>(std::ceil(maxY - minY + marginPx * 2.0f));

		std::vector<uint8_t> image(static_cast<size_t>(widthPx * heightPx * 4), 0);
		Color background{28, 30, 36, 255};
		for (int y = 0; y < heightPx; ++y) {
			for (int x = 0; x < widthPx; ++x) {
				setPixel(image, widthPx, heightPx, x, y, background);
			}
		}

		auto toImage = [&](const glm::vec2& world) -> glm::ivec2 {
			glm::vec2 p = world * hexRadiusPx;
			return glm::ivec2(
				static_cast<int>(std::round(p.x - minX + marginPx)),
				static_cast<int>(std::round(p.y - minY + marginPx)));
		};

		const Color tileOutline{90, 130, 190, 255};
		const Color tileLabel{235, 235, 235, 255};
		const Color edgeColor{60, 210, 190, 255};
		const Color edgeLabel{210, 240, 240, 255};
		const Color vertexColor{230, 170, 80, 255};
		const Color vertexLabel{240, 240, 200, 255};

		// Draw tiles as hex outlines and label with tile ID
		for (size_t tileId : tileIds) {
			const uint32_t row = static_cast<uint32_t>(tileId / graph.getMapWidth());
			const uint32_t col = static_cast<uint32_t>(tileId % graph.getMapWidth());
			glm::vec2 centerWorld = WorldNodeMapper::getTilePosition(row, col);
			glm::ivec2 centerPx = toImage(centerWorld);

			std::array<glm::ivec2, 6> corners{};
			for (size_t i = 0; i < vertexOffsets.size(); ++i)
				corners[i] = toImage(centerWorld + vertexOffsets[i]);

			for (size_t i = 0; i < corners.size(); ++i)
				drawLine(image, widthPx, heightPx, corners[i], corners[(i + 1) % corners.size()], tileOutline);

			drawText(image, widthPx, heightPx, centerPx.x - 2, centerPx.y - (kGlyphHeight / 2), std::to_string(tileId), tileLabel);
		}

		const auto edgeIds = gatherEdgeIds(graph);
		for (size_t edgeId : edgeIds) {
			EdgeHandle edge = graph.findEdgeById(edgeId);
			if (!edge)
				continue;

			const auto vertices = graph.getEdgeVertices(edge);
			if (!vertices || !isValidHandle((*vertices)[0]) || !isValidHandle((*vertices)[1]))
				continue;

			glm::vec2 p0World = WorldNodeMapper::getWorldPositionForVertex((*vertices)[0]->getId(), graph);
			glm::vec2 p1World = WorldNodeMapper::getWorldPositionForVertex((*vertices)[1]->getId(), graph);
			glm::ivec2 p0 = toImage(p0World);
			glm::ivec2 p1 = toImage(p1World);

			drawLine(image, widthPx, heightPx, p0, p1, edgeColor);

			glm::ivec2 mid{(p0.x + p1.x) / 2, (p0.y + p1.y) / 2};
			drawText(image, widthPx, heightPx, mid.x - 1, mid.y - kGlyphHeight - 2, std::to_string(edgeId), edgeLabel);
		}

		const auto vertexIds = gatherVertexIds(graph);
		for (size_t vertexId : vertexIds) {
			VertexHandle vertex = graph.findVertexById(vertexId);
			if (!vertex)
				continue;

			glm::vec2 posWorld = WorldNodeMapper::getWorldPositionForVertex(vertexId, graph);
			glm::ivec2 posPx = toImage(posWorld);

			drawCircleFilled(image, widthPx, heightPx, posPx, 4, vertexColor);
			drawText(image, widthPx, heightPx, posPx.x + 6, posPx.y - (kGlyphHeight / 2), std::to_string(vertexId), vertexLabel);
		}

		// Expand tilde and create parent directory if needed
		std::filesystem::path expandedPath = expandPath(outputPath);
		if (expandedPath.has_parent_path()) {
			std::filesystem::create_directories(expandedPath.parent_path());
		}

		if (!stbi_write_png(expandedPath.string().c_str(), widthPx, heightPx, 4, image.data(), widthPx * 4))
			throw std::runtime_error(fmt::format("Failed to write graph debug image to {}", expandedPath.string()));
	}
} // namespace df::utils

