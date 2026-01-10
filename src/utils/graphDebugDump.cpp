#include "graphDebugDump.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "fmt/format.h"

#include "graph.h"


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

		template <typename T>
		void sortUnique(std::vector<T>& values) {
			std::sort(values.begin(), values.end());
			values.erase(std::unique(values.begin(), values.end()), values.end());
		}

		template <typename Handle>
		bool isValidHandle(Handle handle) {
			return handle != nullptr && handle->getId() != SIZE_MAX;
		}

		void writeIdList(std::ostream& os, const std::vector<size_t>& ids) {
			if (ids.empty()) {
				os << "-";
				return;
			}

			for (size_t i = 0; i < ids.size(); ++i) {
				os << ids[i];
				if (i + 1 < ids.size())
					os << ", ";
			}
		}

		template <typename ArrayT>
		void writeIndexedHandleArray(std::ostream& os, const ArrayT& handles) {
			for (size_t i = 0; i < handles.size(); ++i) {
				os << "[" << i << "]=";
				if (isValidHandle(handles[i])) {
					os << handles[i]->getId();
				} else {
					os << "null";
				}

				if (i + 1 < handles.size())
					os << ", ";
			}
		}

		std::unordered_map<size_t, std::vector<size_t>> buildEdgeToTilesMap(const Graph& graph) {
			std::unordered_map<size_t, std::vector<size_t>> edgeToTiles;

			for (const auto& tilePtr : graph.getTiles()) {
				if (!tilePtr)
					continue;

				const auto tileId = tilePtr->getId();
				const auto tileEdgesOpt = graph.getTileEdges(tilePtr.get());
				if (!tileEdgesOpt.has_value())
					continue;

				for (const auto& edge : *tileEdgesOpt) {
					if (isValidHandle(edge))
						edgeToTiles[edge->getId()].push_back(tileId);
				}
			}

			for (auto& [_, tileIds] : edgeToTiles)
				sortUnique(tileIds);

			return edgeToTiles;
		}

		std::unordered_map<size_t, size_t> buildTileIdToIndex(const Graph& graph) {
			std::unordered_map<size_t, size_t> idToIdx;
			const auto& tiles = graph.getTiles();
			idToIdx.reserve(tiles.size());
			for (size_t idx = 0; idx < tiles.size(); ++idx) {
				if (tiles[idx])
					idToIdx[tiles[idx]->getId()] = idx;
			}
			return idToIdx;
		}

		std::vector<size_t> gatherTileIds(const Graph& graph) {
			std::vector<size_t> tileIds;
			tileIds.reserve(graph.getTiles().size());

			for (const auto& tilePtr : graph.getTiles())
				if (tilePtr)
					tileIds.push_back(tilePtr->getId());

			sortUnique(tileIds);
			return tileIds;
		}

		std::vector<size_t> gatherEdgeIds(const Graph& graph) {
			std::vector<size_t> edgeIds;
			edgeIds.reserve(graph.getEdges().size());

			for (const auto& edgePtr : graph.getEdges())
				if (edgePtr)
					edgeIds.push_back(edgePtr->getId());

			sortUnique(edgeIds);
			return edgeIds;
		}

		std::vector<size_t> gatherVertexIds(const Graph& graph) {
			std::vector<size_t> vertexIds;
			vertexIds.reserve(graph.getVertices().size());

			for (const auto& vertexPtr : graph.getVertices())
				if (vertexPtr)
					vertexIds.push_back(vertexPtr->getId());

			sortUnique(vertexIds);
			return vertexIds;
		}

		struct SharingStats {
			size_t edgesSharedByTwo = 0;
			size_t edgesNotShared = 0;
			size_t edgesSharedByMany = 0;
			size_t verticesSharedByThree = 0;
			size_t verticesSharedByTwo = 0;
			size_t verticesNotShared = 0;
		};

		SharingStats computeSharingStats(const Graph& graph, const std::unordered_map<size_t, std::vector<size_t>>& edgeToTiles) {
			SharingStats stats;

			for (const auto& [_, tiles] : edgeToTiles) {
				if (tiles.size() == 2) {
					stats.edgesSharedByTwo++;
				} else if (tiles.size() == 1) {
					stats.edgesNotShared++;
				} else if (tiles.size() > 2) {
					stats.edgesSharedByMany++;
				}
			}

			for (const auto& vertexPtr : graph.getVertices()) {
				if (!vertexPtr)
					continue;
				const auto vertexTiles = graph.getVertexTiles(vertexPtr.get());
				if (!vertexTiles.has_value())
					continue;

				size_t validCount = 0;
				for (const auto& t : *vertexTiles) {
					if (isValidHandle(t))
						++validCount;
				}

				if (validCount == 3) {
					stats.verticesSharedByThree++;
				} else if (validCount == 2) {
					stats.verticesSharedByTwo++;
				} else if (validCount == 1) {
					stats.verticesNotShared++;
				}
			}

			return stats;
		}

		struct ValidationIssues {
			std::vector<std::string> errors;
			std::vector<std::string> warnings;
		};

		ValidationIssues validateGraph(const Graph& graph, const std::unordered_map<size_t, std::vector<size_t>>& edgeToTiles) {
			ValidationIssues issues;

			// Tile mappings present and non-null counts
			for (const auto& tilePtr : graph.getTiles()) {
				if (!tilePtr)
					continue;
				const size_t tileId = tilePtr->getId();

				const auto tileEdges = graph.getTileEdges(tilePtr.get());
				const auto tileVertices = graph.getTileVertices(tilePtr.get());

				if (!tileEdges.has_value())
					issues.errors.push_back(fmt::format("Tile {} missing edge mapping", tileId));
				if (!tileVertices.has_value())
					issues.errors.push_back(fmt::format("Tile {} missing vertex mapping", tileId));

				if (tileEdges.has_value()) {
					size_t nullEdges = 0;
					for (const auto& e : *tileEdges)
						if (!isValidHandle(e))
							++nullEdges;
					if (nullEdges > 0)
						issues.warnings.push_back(fmt::format("Tile {} has {} null edges", tileId, nullEdges));
				}

				if (tileVertices.has_value()) {
					size_t nullVertices = 0;
					for (const auto& v : *tileVertices)
						if (!isValidHandle(v))
							++nullVertices;
					if (nullVertices > 0)
						issues.warnings.push_back(fmt::format("Tile {} has {} null vertices", tileId, nullVertices));
				}
			}

			// Edge symmetry and orphan checks
			for (const auto& edgePtr : graph.getEdges()) {
				if (!edgePtr)
					continue;
				const size_t edgeId = edgePtr->getId();

				if (!edgeToTiles.contains(edgeId))
					issues.warnings.push_back(fmt::format("Edge {} not referenced by any tile (orphaned)", edgeId));
				else if (edgeToTiles.at(edgeId).size() > 2)
					issues.errors.push_back(fmt::format("Edge {} referenced by {} tiles (>2)", edgeId, edgeToTiles.at(edgeId).size()));

				const auto edgeVertices = graph.getEdgeVertices(edgePtr.get());
				if (!edgeVertices.has_value()) {
					issues.errors.push_back(fmt::format("Edge {} missing vertex mapping", edgeId));
					continue;
				}

				for (const auto& vertex : *edgeVertices) {
					if (!isValidHandle(vertex))
						continue;

					const auto vertexEdges = graph.getVertexEdges(vertex);
					if (!vertexEdges.has_value()) {
						issues.errors.push_back(fmt::format("Edge {} references vertex {} but vertex has no edge mapping", edgeId, vertex->getId()));
						continue;
					}

					bool backLinkFound = false;
					for (const auto& ve : *vertexEdges) {
						if (isValidHandle(ve) && ve->getId() == edgeId) {
							backLinkFound = true;
							break;
						}
					}
					if (!backLinkFound)
						issues.errors.push_back(fmt::format("Edge {} references vertex {} but vertex lacks back-reference", edgeId, vertex->getId()));
				}
			}

			// Vertex symmetry and orphan checks
			for (const auto& vertexPtr : graph.getVertices()) {
				if (!vertexPtr)
					continue;
				const size_t vertexId = vertexPtr->getId();

				const auto vertexTiles = graph.getVertexTiles(vertexPtr.get());
				if (!vertexTiles.has_value())
					issues.errors.push_back(fmt::format("Vertex {} missing tile mapping", vertexId));

				const auto vertexEdges = graph.getVertexEdges(vertexPtr.get());
				if (!vertexEdges.has_value())
					issues.errors.push_back(fmt::format("Vertex {} missing edge mapping", vertexId));

				if (vertexEdges.has_value()) {
					for (const auto& edge : *vertexEdges) {
						if (!isValidHandle(edge))
							continue;

						const auto edgeVertices = graph.getEdgeVertices(edge);
						if (!edgeVertices.has_value()) {
							issues.errors.push_back(fmt::format("Vertex {} references edge {} but edge has no vertex mapping", vertexId, edge->getId()));
							continue;
						}

						bool backLinkFound = false;
						for (const auto& ev : *edgeVertices) {
							if (isValidHandle(ev) && ev->getId() == vertexId) {
								backLinkFound = true;
								break;
							}
						}
						if (!backLinkFound)
							issues.errors.push_back(fmt::format("Vertex {} references edge {} but edge lacks back-reference", vertexId, edge->getId()));
					}
				}

				if (vertexTiles.has_value()) {
					for (const auto& tile : *vertexTiles) {
						if (!isValidHandle(tile))
							continue;

						const auto tileVertices = graph.getTileVertices(tile);
						if (!tileVertices.has_value()) {
							issues.errors.push_back(fmt::format("Vertex {} references tile {} but tile has no vertex mapping", vertexId, tile->getId()));
							continue;
						}

						bool backLinkFound = false;
						for (const auto& tv : *tileVertices) {
							if (isValidHandle(tv) && tv->getId() == vertexId) {
								backLinkFound = true;
								break;
							}
						}
						if (!backLinkFound)
							issues.errors.push_back(fmt::format("Vertex {} references tile {} but tile lacks back-reference", vertexId, tile->getId()));
					}
				}
			}

			return issues;
		}
	} // namespace


	void writeGraphDebugDump(const Graph& graph, const std::filesystem::path& outputPath) {
		// Expand tilde and create parent directory if needed
		std::filesystem::path expandedPath = expandPath(outputPath);
		if (expandedPath.has_parent_path()) {
			std::filesystem::create_directories(expandedPath.parent_path());
		}

		std::ofstream out(expandedPath);
		if (!out.is_open()) {
			throw std::runtime_error(fmt::format("Failed to open graph debug output file: {}", expandedPath.string()));
		}

		fmt::println("[DEBUG graphDebugDump] Graph has {} tiles, {} edges, {} vertices",
			graph.getTiles().size(), graph.getEdges().size(), graph.getVertices().size());

		const auto tileIds = gatherTileIds(graph);
		const auto edgeIds = gatherEdgeIds(graph);
		const auto vertexIds = gatherVertexIds(graph);
		const auto edgeToTiles = buildEdgeToTilesMap(graph);
		const auto tileIdToIndex = buildTileIdToIndex(graph);
		const auto sharingStats = computeSharingStats(graph, edgeToTiles);
		const auto validation = validateGraph(graph, edgeToTiles);

		const std::string majorSep(70, '=');
		const std::string minorSep(70, '-');

		out << majorSep << '\n';
		out << "GRAPH DEBUG DUMP\n";
		out << majorSep << "\n\n";

		out << "SUMMARY:\n";
		out << "  Tiles    : " << graph.getTileCount() << '\n';
		out << "  Edges    : " << graph.getEdgeCount() << '\n';
		out << "  Vertices : " << graph.getVertexCount() << '\n';
		if (graph.getMapWidth() > 0) {
			const size_t rows = (graph.getMapWidth() == 0) ? 0 : (graph.getTileCount() / graph.getMapWidth());
			out << "  Map Width: " << graph.getMapWidth() << '\n';
			out << "  Map Rows : " << rows << '\n';
		}
		out << '\n';

		out << "SHARING STATISTICS:\n";
		out << "  Edges shared by 2 tiles   : " << sharingStats.edgesSharedByTwo << '\n';
		out << "  Edges not shared (border) : " << sharingStats.edgesNotShared << '\n';
		out << "  Edges shared by >2 tiles  : " << sharingStats.edgesSharedByMany << '\n';
		out << "  Vertices shared by 3 tiles: " << sharingStats.verticesSharedByThree << '\n';
		out << "  Vertices shared by 2 tiles: " << sharingStats.verticesSharedByTwo << '\n';
		out << "  Vertices not shared       : " << sharingStats.verticesNotShared << '\n';
		out << '\n';

		if (!validation.errors.empty() || !validation.warnings.empty()) {
			out << "VALIDATION ISSUES:\n";
			if (!validation.errors.empty()) {
				out << "  ERRORS (" << validation.errors.size() << "):\n";
				for (const auto& err : validation.errors)
					out << "    - " << err << '\n';
			}
			if (!validation.warnings.empty()) {
				out << "  WARNINGS (" << validation.warnings.size() << "):\n";
				for (const auto& warn : validation.warnings)
					out << "    - " << warn << '\n';
			}
			out << '\n';
		} else {
			out << "VALIDATION: All checks passed\n\n";
		}

		out << minorSep << '\n';
		out << "TILES (edge/vertex indices order: 0=NE, 1=E, 2=SE, 3=SW, 4=W, 5=NW)\n";
		out << minorSep << '\n';

		for (const auto tileId : tileIds) {
			TileHandle tile = graph.findTileById(tileId);
			if (!tile)
				continue;

			out << "\nTile " << std::setw(6) << tileId;
			if (graph.getMapWidth() > 0) {
				if (auto it = tileIdToIndex.find(tileId); it != tileIdToIndex.end()) {
					const size_t idx = it->second;
					const size_t row = idx / graph.getMapWidth();
					const size_t col = idx % graph.getMapWidth();
					out << "  (row=" << row << ", col=" << col << ")";
				}
			}
			out << "  type=" << static_cast<int>(tile->getType()) << " potency=" << static_cast<int>(tile->getPotency()) << '\n';

			if (const auto tileEdges = graph.getTileEdges(tile); tileEdges.has_value()) {
				out << "  Edges    : ";
				writeIndexedHandleArray(out, *tileEdges);
				out << '\n';
			} else {
				out << "  Edges    : <mapping not found>\n";
			}

			if (const auto tileVertices = graph.getTileVertices(tile); tileVertices.has_value()) {
				out << "  Vertices : ";
				writeIndexedHandleArray(out, *tileVertices);
				out << '\n';
			} else {
				out << "  Vertices : <mapping not found>\n";
			}
		}

		out << "\n" << minorSep << '\n';
		out << "EDGES\n";
		out << minorSep << '\n';

		for (const auto edgeId : edgeIds) {
			EdgeHandle edge = graph.findEdgeById(edgeId);
			if (!edge)
				continue;

			out << "\nEdge " << std::setw(7) << edgeId << '\n';
			out << "  Tiles    : ";
			if (const auto it = edgeToTiles.find(edgeId); it != edgeToTiles.end()) {
				writeIdList(out, it->second);
				if (it->second.size() == 1)
					out << "  (border)";
				else if (it->second.size() > 2)
					out << "  (ERROR: >2 tiles)";
			} else {
				out << "- (orphan)";
			}
			out << '\n';

			if (const auto edgeVertices = graph.getEdgeVertices(edge); edgeVertices.has_value()) {
				out << "  Vertices : ";
				writeIndexedHandleArray(out, *edgeVertices);
				out << '\n';
			} else {
				out << "  Vertices : <mapping not found>\n";
			}
		}

		out << "\n" << minorSep << '\n';
		out << "VERTICES\n";
		out << minorSep << '\n';

		for (const auto vertexId : vertexIds) {
			VertexHandle vertex = graph.findVertexById(vertexId);
			if (!vertex)
				continue;

			out << "\nVertex " << std::setw(7) << vertexId << '\n';

			if (const auto vertexTiles = graph.getVertexTiles(vertex); vertexTiles.has_value()) {
				out << "  Tiles    : ";
				writeIndexedHandleArray(out, *vertexTiles);
				size_t validCount = 0;
				for (const auto& t : *vertexTiles)
					if (isValidHandle(t))
						++validCount;
				if (validCount == 1)
					out << "  (corner)";
				else if (validCount == 2)
					out << "  (border)";
				out << '\n';
			} else {
				out << "  Tiles    : <mapping not found>\n";
			}

			if (const auto vertexEdges = graph.getVertexEdges(vertex); vertexEdges.has_value()) {
				out << "  Edges    : ";
				writeIndexedHandleArray(out, *vertexEdges);
				out << '\n';
			} else {
				out << "  Edges    : <mapping not found>\n";
			}
		}

		out << '\n' << majorSep << '\n';
		out << "END OF DUMP\n";
		out << majorSep << '\n';
	}
} // namespace df::utils


