#pragma once

#include <filesystem>

namespace df {
	class Graph;

	namespace utils {

		/**
		 * Writes a human readable dump of the graph topology to the given file.
		 * The output contains tile, edge and vertex IDs plus their connections.
		 * Throws std::runtime_error if the file cannot be opened.
		 */
		void writeGraphDebugDump(const Graph& graph, const std::filesystem::path& outputPath);

	} // namespace utils
} // namespace df




