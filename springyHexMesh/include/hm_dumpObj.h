#pragma once

/*

This file is part of the SpringHexMesh Project.

	The SpringHexMesh Project is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	The SpringHexMesh Project is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This link provides the exact terms of the GPL license <https://www.gnu.org/licenses/>.

	The author's interpretation of GPL 3 is that if you receive money for the use or distribution of the TriMesh Library or a derivative product, GPL 3 no longer applies.

	Under those circumstances, the author expects and may legally pursue a reasoble share of the income. To avoid the complexity of agreements and negotiation, the author makes
	no specific demands in this regard. Compensation of roughly 1% of net or $5 per user license seems appropriate, but is not legally binding.

	In lay terms, if you make a profit by using the SpringHexMesh Project (violating the spirit of Open Source Software), I expect a reasonable share for my efforts.

	Robert R Tipton - Author

	Dark Sky Innovative Solutions http://darkskyinnovation.com/

*/

#include <tm_defines.h>

#include <vector>
#include <map>
#include <string>
#include <iostream>

#include <hm_grid.h>

namespace HexahedralMesher {

	class GridFace;

	class DumpObj {
	public:
		DumpObj(const Grid& grid, const std::string& path);

		void write(std::ostream& out, int minNumberOfClamps = 0, int clampMask = -1) const;
		void write(const std::string& filename, int minNumberOfClamps = 0, int clampMask = -1) const;

		void writeFaces(std::ostream& out, const std::vector<GridFace>& faces) const;
		void writeFaces(std::ostream& out, int minNumberOfClamps = 0, int clampMask = -1) const;
		void writeFaces(const std::string& filename, int minNumberOfClamps = 0, int clampMask = -1) const;

		void write(const std::string& filename, const std::vector<Vector3d>& pts) const;

		void writeCells(std::ostream& out, const std::vector<size_t>& cellIndices) const;
		void writeCells(const std::string& filename, const std::vector<size_t>& cellIndices) const;

	private:
		void gatherClampedCells(int minNumberOfClamps, int clampMask,
			std::vector<size_t>& cellIndices) const;

		void gatherClampedFaces(int minNumberOfClamps, int clampMask,
			std::vector<GridFace>& faces) const;

		void gatherEdges(const std::vector<size_t> cellIndices, std::set<GridEdge>& edges) const;
		void gatherEdges(const std::vector<GridFace> faces, std::set<GridEdge>& edges) const;

		void gatherFaces(const std::vector<size_t> cellIndices, std::set<SearchableFace>& faceSet) const;
		void gatherFaces(const std::vector<GridFace> faces, std::set<SearchableFace>& faceSet) const;

		void createMaps(const std::vector<size_t>& faces,
			std::vector<size_t>& vertIndices,
			std::map<size_t, size_t>& vertIdxMap) const;

		void createMaps(const std::vector<GridFace>& cellIndices,
			std::vector<size_t>& vertIndices,
			std::map<size_t, size_t>& vertIdxMap) const;

		void dumpVertsObj(std::ostream& out, const std::vector<size_t>& vertIndices) const;
		void dumpLinesObj(std::ostream& out, const std::set<GridEdge>& edges, const std::map<size_t, size_t>& vertIdxMap) const;
		void dumpFacesObj(std::ostream& out, const std::set<SearchableFace>& faceSet, const std::map<size_t, size_t>& vertIdxMap) const;

		void dumpCellObj(std::string& filename, size_t cellIdx) const;
		void dumpCellObj(std::ostream& out, size_t cellIdx) const;

		const std::string _path;
		const Grid& _grid;
	};
}