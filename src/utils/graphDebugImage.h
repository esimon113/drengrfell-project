#pragma once

#include <filesystem>

namespace df {
	class Graph;

	namespace utils {

		/**
		 * Renders an annotated PNG of the graph topology (tiles, edges, vertices).
		 * The image visualizes the axial hex layout and writes IDs near each element.
		 * Throws std::runtime_error if the file cannot be written or the map is invalid.
		 */
		void writeGraphDebugImage(const Graph& graph, const std::filesystem::path& outputPath);

	} // namespace utils
} // namespace df

