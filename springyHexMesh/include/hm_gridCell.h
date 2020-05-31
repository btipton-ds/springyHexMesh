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

#include <tm_defines.h>

#include <memory>
#include <map>
#include <set>
#include <vector>
#include <iostream>

#include <hm_types.h>
#include <hm_topolRef.h>

namespace TriMesh {
	class CEdge;
	class CMesh;
	using CMeshPtr = std::shared_ptr<CMesh>;
}


namespace HexahedralMesher {

	class GridBase;

	class GridCell {
	public:
		static CellVertPos vertPosOf(const Vector3i& vi);
		static CellVertPos getOppCorner(CellVertPos pos);
		static FaceNumber getOppFace(FaceNumber face);
		static void getAdjacentEdgeEnds(CellVertPos pos, CellVertPos others[3]);
		static void getVertFaces(CellVertPos pos, FaceNumber faces[3]);
		static void getVertGridFaces(size_t thisCellIdx, CellVertPos pos, GridFace faces[3]);
		static void getFaceCellPos(FaceNumber faceNumber, CellVertPos cornerPos[4]);
		static void getTriCellPos(FaceNumber faceNumber, CellVertPos triPos[2][3]);

		GridCell();
		GridCell(const GridCell& src) = default;
		~GridCell();
		void save(std::ostream& out) const;
		bool readVersion1(std::istream& in);

		bool verify(const GridBase& grid, bool verifyVerts) const;

		size_t getId() const;
		size_t getVertIdx(CellVertPos idx) const;
		void setVertIdx(CellVertPos idx, size_t vertIdx);
		const GridVert& getVert(const Grid& grid, CellVertPos idx) const;
		GridVert& getVert(Grid& grid, CellVertPos idx);
		CellVertPos getVertsPos(size_t vertIdx) const;
		CellVertPos getClosestVertsPos(const Grid& grid, const LineSegment& seg, double& dist) const;
		CellVertPos getVertsEdgeEndPos(CellVertPos pos, VertEdgeDir edgeDir) const;
		size_t getVertsEdgeEndVertIdx(CellVertPos pos, VertEdgeDir edgeDir) const;
		size_t getOppositeEdgeEndVertIdx(FaceNumber faceNumber, CellVertPos corner);
		void getVertsEdgeIndices(CellVertPos pos, size_t edgeVertIndices[3]) const;
		GridEdge getEdge(size_t edgeNumber) const;

		double getRestEdgeLength(int i) const;
		void setRestEdgeLength(int i, double val);
		void setDefaultRestEdgeLengths(const Grid& grid);

		void getFaceIndices(FaceNumber faceNumber, size_t indices[4]) const;
		void getFaceTriIndices(FaceNumber faceNumber, size_t tri[2][3]) const;
		void getFacePoints(const Grid& grid, FaceNumber faceNumber, const Vector3d* points[4]) const;
		void getFaceTriPoints(const GridBase& grid, FaceNumber faceNumber, const Vector3d* triPoints[2][3]) const;
		bool isPerpendicularBoundaryFace(const Grid& grid, FaceNumber faceNumber, TopolRef& clamp) const;
		double distToFace(const Grid& grid, FaceNumber faceNumber, const Vector3d& pt) const;

		CBoundingBox3Dd calcBBox(const Grid& grid) const;
		size_t calcEdgeLengths(const Grid& grid, std::map<GridEdge, double>& lengths) const;

		double calcVolume(const GridBase& grid) const;

		int getNumClamped(const Grid& grid, int clampMask = -1) const;
		bool containsPoint(const Grid& grid, const Vector3d& pt) const;
		Vector3d calcCentroid(const Grid& grid) const;
		Vector3d calcFaceCentroid(const Grid& grid, FaceNumber fn) const;

		void dump(std::ostream& out) const;

	private:
		friend class GridBase;

		void attach(GridBase& grid);
		void detach(GridBase& grid);

		void updateVertChangeNumbers(const Grid& grid) const;

		size_t _id = stm1;
		size_t _vertIndices[8];
		double _restEdgeLen[12];
	};

	inline size_t GridCell::getId() const {
		return _id;
	}

	inline CellVertPos GridCell::vertPosOf(const Vector3i& vi) {
		return (CellVertPos)(vi[Z_AXIS] << 2 | vi[Y_AXIS] << 1 | vi[X_AXIS]);
	}

	inline size_t GridCell::getVertIdx(CellVertPos idx) const {
		return _vertIndices[idx];
	}

	inline void GridCell::setVertIdx(CellVertPos idx, size_t vertIdx) {
		_vertIndices[idx] = vertIdx;
	}

	inline double GridCell::getRestEdgeLength(int i) const {
		return _restEdgeLen[i];
	}

	inline void GridCell::setRestEdgeLength(int i, double val) {
		_restEdgeLen[i] = val;
	}

}
