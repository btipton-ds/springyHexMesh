#pragma once

/*

This file is part of the EnerMesh Project.

	The EnerMesh Project is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	The EnerMesh Project is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This link provides the exact terms of the GPL license <https://www.gnu.org/licenses/>.

	The author's interpretation of GPL 3 is that if you receive money for the use or distribution of the TriMesh Library or a derivative product, GPL 3 no longer applies.

	Under those circumstances, the author expects and may legally pursue a reasoble share of the income. To avoid the complexity of agreements and negotiation, the author makes
	no specific demands in this regard. Compensation of roughly 1% of net or $5 per user license seems appropriate, but is not legally binding.

	In lay terms, if you make a profit by using the EnerMesh Project (violating the spirit of Open Source Software), I expect a reasonable share for my efforts.

	Robert R Tipton - Author

	Dark Sky Innovative Solutions http://darkskyinnovation.com/

*/

#pragma once

#include <tm_defines.h>

#include <vector>
#include <map>
#include <set>

#include <hm_types.h>
#include <hm_gridEdge.h>
#include <hm_gridCell.h>

namespace HexahedralMesher {

	class Grid;
	class GridEdge;

	class CSplitter {
	public:
		CSplitter(Grid& grid);
		~CSplitter();

		void splitAll();
		void splitCellFull(size_t cellIdx);
		void splitCells(std::set<size_t> cells);

		const std::vector<size_t>& getNewCells() const {
			return _newCells;
		}

		std::vector<size_t> getClampedCells() const;

	private:
		enum SplitType {
			SPLIT_OCT,
			SPLIT_TRI_PRISM,
			SPLIT_UNKNOWN
		};

		friend struct SplitSourceRec;
		struct SplitSourceRec;
		struct SplitCellDiagonallyRec;

		void setPoint(size_t vertIdx, const Vector3d& pt);
		SplitSourceRec& createOctSplit(size_t cellId);
		SplitSourceRec& createTriPrismSplit(size_t cellId, FaceNumber faceNumber, CellVertPos corner0, CellVertPos corner1);

		void clear();
		void postSplit();
		void fixBrokenLinks();
		void splitCellFullInit(size_t cellIdx);

		size_t add(const Vector3d& pt);
		size_t addCell(GridCell& cell, SplitSourceRec& splitRec);
		size_t corn(CellVertPos p) const;
		size_t edge(CellVertPos p0, CellVertPos p1);
		size_t face(FaceNumber fn) const;

		bool splitCellDiagonally(size_t cellIdx, std::map<GridEdge, std::vector<SplitCellDiagonallyRec>>& cellPairsToSplit);

		const GridVert& getVert(size_t vertIdx) const;
		GridVert& getVert(size_t vertIdx);
		const GridCell& getCell(size_t cellIdx) const;
		GridCell& getCell(size_t cellIdx);
		bool isClamped(size_t vertIdx) const;
		bool needsClamp(size_t vertIdx) const;

		void splitCellFaceDiagonally(size_t cellIdx, FaceNumber faceNumber, CellVertPos corner0, CellVertPos corner1);
		void addTriangluarPrism(const size_t tri0[3], const size_t tri1[3], SplitSourceRec& splitRec);
		size_t addQuad(const size_t frontFace[4], const size_t backFace[4], SplitSourceRec& splitRec);
		void addQuadPrism(const size_t frontFace[4], const size_t backFace[4], SplitSourceRec& splitRec);

		size_t addSubCellLwrFntLft(SplitSourceRec& splitRec);
		size_t addSubCellLwrFntRgt(SplitSourceRec& splitRec);
		size_t addSubCellLwrBckLft(SplitSourceRec& splitRec);
		size_t addSubCellLwrBckRgt(SplitSourceRec& splitRec);

		size_t addSubCellUprFntLft(SplitSourceRec& splitRec);
		size_t addSubCellUprFntRgt(SplitSourceRec& splitRec);
		size_t addSubCellUprBckLft(SplitSourceRec& splitRec);
		size_t addSubCellUprBckRgt(SplitSourceRec& splitRec);

		void buildEdgeSet();
		void clampToBoundaries(SplitSourceRec& splitRec);
		void clampToAdjacentCells(SplitSourceRec& splitRec);
		void clampToAdjacentCellEdge(SplitSourceRec& splitRec, int edgeNum);
		void clampToAdjacentCellFace(SplitSourceRec& splitRec, FaceNumber faceNum);

		void clampToEdgeBoundaries(SplitSourceRec& splitRec);
		void clampToFaceBoundaries(SplitSourceRec& splitRec);
		void applyOctSplitClamps(SplitSourceRec& splitRec);
		void applyTriPrismSplitClamps(SplitSourceRec& splitRec);
		bool clampedToSamePolyline(size_t vert0, size_t vert, size_t& meshIdx, size_t& polylineNum) const;

		bool clampVertToPolyline(size_t vertIdx, size_t meshIdx, size_t polylineNum);
		bool clampVertToDeletedBoundaryEdge(size_t vertIdx, const GridEdge& edge);
		bool clampVertToCellEdgeMidPoint(size_t vertIdx, const GridEdge& edge);
		bool clampVertToCellEdgeMidPoints(size_t vertIdx);

		bool clampVertToFaceTriVerts(size_t vertIdx);
		bool clampVertToGridBoundary(size_t vertIdx);
		bool getAdjacentVerts(size_t vertIdx, std::set<size_t>& edgeAdj, std::set<size_t>& cornerAdj) const;
		bool clampVertToCellEdgeMidPoint(size_t vertIdx);

		size_t _center;
		size_t _faceCenters[6];
		size_t _numInitialVerts;
		std::map<GridEdge, size_t> _edgeCenters;

		Grid& _grid;
		GridCell _workCell;
		std::vector<size_t> _clampedVerts, _newCells;
		std::vector<SplitSourceRec> _cellsToClamp;
		std::set<GridEdge> _finalEdgeSet;

		std::map<size_t, Vector3d> _changedPointMap;
	};

}
