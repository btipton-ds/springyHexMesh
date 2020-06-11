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

#include <iostream>
#include <string>
#include <algorithm>

#include <tm_edge.h>
#include <triMesh.h>
#include <hm_gridCell.h>

#include <hm_gridVert.h>
#include <hm_gridEdge.h>
#include <hm_grid.h>
#include <hm_tables.h>
#include <hm_dump.h>

namespace HexahedralMesher {

	using namespace std;


	CellVertPos GridCell::getOppCorner(CellVertPos pos) {
		return gOppCornerLUT[pos];
	}

	FaceNumber GridCell::getOppFace(FaceNumber face) {
		return gOppFaceLUT[face];
	}

	size_t GridCell::getOppositeEdgeEndVertIdx(FaceNumber faceNumber, CellVertPos corner) {
		switch (faceNumber) {
		case BOTTOM:
			return getVertsEdgeEndVertIdx(corner, Z_POS);
		case TOP:
			return getVertsEdgeEndVertIdx(corner, Z_NEG);
		case FRONT:
			return getVertsEdgeEndVertIdx(corner, Y_POS);
		case BACK:
			return getVertsEdgeEndVertIdx(corner, Y_NEG);
		case LEFT:
			return getVertsEdgeEndVertIdx(corner, X_POS);
		case RIGHT:
			return getVertsEdgeEndVertIdx(corner, X_NEG);
		default:
			break;
		}
		return stm1;
	}


	void GridCell::getAdjacentEdgeEnds(CellVertPos pos, CellVertPos others[3]) {
		CellVertPos const * const LUT = gPosEdgePosLT[pos];
		int idx = 0;
		for (int i = 0; i < 6; i++) {
			if (LUT[i] != CVP_UNKNOWN)
				others[idx++] = LUT[i];
		}
		if (idx != 3)
			throw "Unexpected condition, didn't get 3 positions.";
	}

	void GridCell::getVertGridFaces(size_t thisCellIdx, CellVertPos pos, GridFace faces[3]) {
		for (int i = 0; i < 3; i++)
			faces[i] = GridFace(thisCellIdx, gPosFaceNumberLUT[pos][i]);
	}

	void GridCell::getFaceCellPos(FaceNumber faceNumber, CellVertPos cornerPos[4]) {
		for (int i = 0; i < 4; i++) {
			cornerPos[i] = gFaceIdxLUT[faceNumber][i];
		}
	}

	void GridCell::getTriCellPos(FaceNumber faceNumber, CellVertPos triPos[2][3]) {
		CellVertPos cornerPos[4];
		getFaceCellPos(faceNumber, cornerPos);

		triPos[0][0] = cornerPos[0];
		triPos[0][1] = cornerPos[1];
		triPos[0][2] = cornerPos[2];

		triPos[1][0] = cornerPos[0];
		triPos[1][1] = cornerPos[2];
		triPos[1][2] = cornerPos[3];

	}

	void GridCell::getVertFaces(CellVertPos pos, FaceNumber faces[3]) {
		for (int i = 0; i < 3; i++)
			faces[i] = gPosFaceNumberLUT[pos][i];
	}

	void getVertsEdgeIndices(CellVertPos pos, size_t edgeVertIndices[3]);

	GridCell::GridCell() {
		for (int i = 0; i < 8; i++) 
			_vertIndices[i] = stm1;
		for (int i = 0; i < 12; i++)
			_restEdgeLen[i] = -1;
	}

	GridCell::~GridCell() {
	}

	void GridCell::attach(GridBase& grid) {
		for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
			GridVert& vert = grid.getVert(_vertIndices[p]);
			vert.addCellIndex(_id);
		}
	}

	void GridCell::detach(GridBase& grid) {
		for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
			GridVert& vert = grid.getVert(_vertIndices[p]);
			vert.removeCellIndex(_id);
		}
	}

	bool GridCell::verify(const GridBase& grid, bool verifyVerts) const {
		double vol = calcVolume(grid);
		if (vol <= SAME_DIST_TOL * SAME_DIST_TOL * SAME_DIST_TOL || vol > 1.5) {
			cout << _id << " bad volume\n";
			return false;
		}

		for (int i = 0; i < 12; i++) {
			if (_restEdgeLen[i]< 1.0e-9) {
				cout << _id << " _restEdgeLen not set\n";
				return false;
			}

		}

		for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
			if (verifyVerts) {
				size_t vert0Idx = _vertIndices[p];
				const GridVert& vert0 = grid.getVert(vert0Idx);

				// Each position in the vertex can only be occupied by one cell, this one
				if (!vert0.verify(grid)) {
					cout << _id << " bad vertex\n";
					return false;
				}
			}


			switch (p) {
			case LWR_FNT_LFT:
				if (getVertsEdgeEndVertIdx(p, X_POS) != _vertIndices[LWR_FNT_RGT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_POS) != _vertIndices[LWR_BCK_LFT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_POS) != _vertIndices[UPR_FNT_LFT])
					return false;

				if (getVertsEdgeEndVertIdx(p, X_NEG) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_NEG) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_NEG) != stm1)
					return false;

				break;
			case LWR_FNT_RGT:
				if (getVertsEdgeEndVertIdx(p, X_NEG) != _vertIndices[LWR_FNT_LFT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_POS) != _vertIndices[LWR_BCK_RGT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_POS) != _vertIndices[UPR_FNT_RGT])
					return false;

				if (getVertsEdgeEndVertIdx(p, X_POS) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_NEG) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_NEG) != stm1)
					return false;

				break;
			case LWR_BCK_LFT:
				if (getVertsEdgeEndVertIdx(p, X_POS) != _vertIndices[LWR_BCK_RGT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_NEG) != _vertIndices[LWR_FNT_LFT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_POS) != _vertIndices[UPR_BCK_LFT])
					return false;

				if (getVertsEdgeEndVertIdx(p, X_NEG) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_POS) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_NEG) != stm1)
					return false;

				break;
			case LWR_BCK_RGT:
				if (getVertsEdgeEndVertIdx(p, X_NEG) != _vertIndices[LWR_BCK_LFT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_NEG) != _vertIndices[LWR_FNT_RGT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_POS) != _vertIndices[UPR_BCK_RGT])
					return false;

				if (getVertsEdgeEndVertIdx(p, X_POS) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_POS) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_NEG) != stm1)
					return false;

				break;

			case UPR_FNT_LFT:
				if (getVertsEdgeEndVertIdx(p, X_POS) != _vertIndices[UPR_FNT_RGT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_POS) != _vertIndices[UPR_BCK_LFT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_NEG) != _vertIndices[LWR_FNT_LFT])
					return false;

				if (getVertsEdgeEndVertIdx(p, X_NEG) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_NEG) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_POS) != stm1)
					return false;

				break;
			case UPR_FNT_RGT:
				if (getVertsEdgeEndVertIdx(p, X_NEG) != _vertIndices[UPR_FNT_LFT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_POS) != _vertIndices[UPR_BCK_RGT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_NEG) != _vertIndices[LWR_FNT_RGT])
					return false;

				if (getVertsEdgeEndVertIdx(p, X_POS) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_NEG) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_POS) != stm1)
					return false;

				break;
			case UPR_BCK_LFT:
				if (getVertsEdgeEndVertIdx(p, X_POS) != _vertIndices[UPR_BCK_RGT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_NEG) != _vertIndices[UPR_FNT_LFT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_NEG) != _vertIndices[LWR_BCK_LFT])
					return false;

				if (getVertsEdgeEndVertIdx(p, X_NEG) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_POS) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_POS) != stm1)
					return false;

				break;
			case UPR_BCK_RGT:
				if (getVertsEdgeEndVertIdx(p, X_NEG) != _vertIndices[UPR_BCK_LFT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_NEG) != _vertIndices[UPR_FNT_RGT])
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_NEG) != _vertIndices[LWR_BCK_RGT])
					return false;

				if (getVertsEdgeEndVertIdx(p, X_POS) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Y_POS) != stm1)
					return false;
				if (getVertsEdgeEndVertIdx(p, Z_POS) != stm1)
					return false;

				break;

			}

		}
		return true;
	}

	void GridCell::save(ostream& out) const {
		out << "ID: " << _id << "\n";

		for (int i = 0; i < 12; i++) {
			if (_restEdgeLen[i] < 1.0e-6)
				throw "Bad cell rest edge length";
		}
		out << "REL: ";
		for (int i = 0; i < 12; i++)
			out << fixed << setprecision(filePrecision) << _restEdgeLen[i] << " ";
		out << "\n";

		out << "VI: ";
		for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
			size_t val = _vertIndices[p];
			out << val << " ";
		}
		out << "\n";
	}

	const GridVert& GridCell::getVert(const Grid& grid, CellVertPos idx) const {
		auto vertIdx = getVertIdx(idx);
		return grid.getVert(vertIdx);
	}

	GridVert& GridCell::getVert(Grid& grid, CellVertPos idx) {
		auto vertIdx = getVertIdx(idx);
		return grid.getVert(vertIdx);
	}

	CBoundingBox3Dd GridCell::calcBBox(const Grid& grid) const {
		BoundingBox bbox;
		for (CellVertPos i = LWR_FNT_LFT; i < CVP_UNKNOWN; i++) {
			const auto& vert = getVert(grid, i);
			bbox.merge(vert.getPt());
		}
		return bbox;
	}

	size_t GridCell::calcEdgeLengths(const Grid& grid, map<GridEdge, double>& lengths) const {
		for (int i = 0; i < 12; i++) {
			GridEdge edge(_vertIndices[gEdgeVerts[i][0]], _vertIndices[gEdgeVerts[i][1]]);
			const auto& pt0 = grid.getVert(edge.getVert(0)).getPt();
			const auto& pt1 = grid.getVert(edge.getVert(1)).getPt();

			double len = (pt1 - pt0).norm();
			lengths.insert(make_pair(edge, len));
		}
		return lengths.size();
	}

	double GridCell::calcVolume(const GridBase& grid) const {
		double vol = 0;
		for (FaceNumber fn = BOTTOM; fn < FN_UNKNOWN; fn++) {
			const Vector3d* tris[2][3];
			getFaceTriPoints(grid, fn, tris);

			for (int i = 0; i < 2; i++) {
				vol += volumeUnderTriangle(tris[i], vZ);
			}
		}
		return vol;
	}

	int GridCell::getNumClamped(const Grid& grid, int clampMask) const {
		int result = 0;
		for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
			const auto& curClamp = grid.getVert(_vertIndices[p]).getClamp();
			if (curClamp.matches(clampMask))
				result++;
		}

		return result;
	}

	bool GridCell::containsPoint(const Grid& grid, const Vector3d& pt) const {
		for (FaceNumber fn = BOTTOM; fn < FN_UNKNOWN; fn++) {
			const Vector3d* triPts[2][3];
			getFaceTriPoints(grid, fn, triPts);
			for (int i = 0; i < 2; i++) {
				Vector3d v0 = (*triPts[i][1] - *triPts[i][0]);
				Vector3d v1 = (*triPts[i][2] - *triPts[i][0]);
				Vector3d normal = v0.cross(v1).normalized();
				Vector3d v2 = pt - *triPts[i][0];
				double d = v2.dot(normal);
				if (d > SAME_DIST_TOL)
					return false;
			}
		}

		return true;
	}

	Vector3d GridCell::calcCentroid(const Grid& grid) const {
		Vector3d result(0, 0, 0);

		for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
			result += grid.getVert(_vertIndices[p]).getPt();
		}
		result /= 8;
		return result;
	}

	Vector3d GridCell::calcFaceCentroid(const Grid& grid, FaceNumber fn) const {
		const Vector3d* facePts[4];
		getFacePoints(grid, fn, facePts);
		Vector3d result(0, 0, 0);
		for (int i = 0; i < 4; i++)
			result += *facePts[i];
		result /= 4;
		return result;
	}

	CellVertPos GridCell::getVertsPos(size_t vertIdx) const {
		for (CellVertPos pos = LWR_FNT_LFT; pos < CVP_UNKNOWN; pos++) {
			if (_vertIndices[pos] == vertIdx)
				return pos;
		}
		return CVP_UNKNOWN;
	}

	CellVertPos GridCell::getClosestVertsPos(const Grid& grid, const LineSegment& seg, double& minDist) const {
		CellVertPos result = CVP_UNKNOWN;

		minDist = DBL_MAX;
		for (CellVertPos i = LWR_FNT_LFT; i < CVP_UNKNOWN; i++) {
			const auto& cPt = grid.getVert(_vertIndices[i]).getPt();
			double d = seg.distanceToPoint(cPt);
			if (d < minDist) {
				minDist = d;
				result = i;
				if (minDist < SAME_DIST_TOL) {
					return result;
				}
			}
		}
		return result;
	}


	CellVertPos GridCell::getVertsEdgeEndPos(CellVertPos pos, VertEdgeDir edgeDir) const {
		return gPosEdgePosLT[pos][edgeDir];
	}

	size_t GridCell::getVertsEdgeEndVertIdx(CellVertPos pos, VertEdgeDir edgeDir) const {
		CellVertPos pos2 = getVertsEdgeEndPos(pos, edgeDir);
		return (pos2 != CVP_UNKNOWN) ? _vertIndices[pos2] : stm1;
	}

	void GridCell::getVertsEdgeIndices(CellVertPos pos, size_t edgeVertIndices[3]) const {
		int count = 0;
		for (int i = 0; i < 6; i++) {
			CellVertPos p;
			if ((p = gPosEdgePosLT[pos][i]) != CVP_UNKNOWN) {
				edgeVertIndices[count++] = _vertIndices[p];
			}
		}
		if (count != 3)
			throw "Bad vert to vert LUT";
	}

	GridEdge GridCell::getEdge(size_t edgeNumber) const {
		return GridEdge(_vertIndices[gEdgeVerts[edgeNumber][0]], _vertIndices[gEdgeVerts[edgeNumber][1]]);
	}

	void GridCell::setDefaultRestEdgeLengths(const Grid& grid) {

		const double k = 1.125; // Bogus correction factor. One reason to avoid this function

		GridEdge edges[4];
		auto setMin = [&]() {
			double minLen = DBL_MAX;
			for (const auto& edge : edges) {
				double l = edge.calcLength(grid);
				if (l < minLen)
					minLen = l;
			}

			for (const auto& edge : edges) {
				for (int i = 0; i < 12; i++) {
					GridEdge testEdge = getEdge(i);
					if (testEdge == edge)
						setRestEdgeLength(i, k * minLen);
				}
			}
		};

		// set front to back edges to the same value
		edges[0] = GridEdge(getVertIdx(LWR_FNT_LFT), getVertIdx(LWR_BCK_LFT));
		edges[1] = GridEdge(getVertIdx(LWR_FNT_RGT), getVertIdx(LWR_BCK_RGT));
		edges[2] = GridEdge(getVertIdx(UPR_FNT_LFT), getVertIdx(UPR_BCK_LFT));
		edges[3] = GridEdge(getVertIdx(UPR_FNT_RGT), getVertIdx(UPR_BCK_RGT));
		setMin();

		// set top to bottom edges to the same value
		edges[0] = GridEdge(getVertIdx(LWR_FNT_LFT), getVertIdx(UPR_FNT_LFT));
		edges[1] = GridEdge(getVertIdx(LWR_FNT_RGT), getVertIdx(UPR_FNT_RGT));
		edges[2] = GridEdge(getVertIdx(LWR_BCK_LFT), getVertIdx(UPR_BCK_LFT));
		edges[3] = GridEdge(getVertIdx(LWR_BCK_RGT), getVertIdx(UPR_BCK_RGT));
		setMin();

		// set left to right edges to the same value
		edges[0] = GridEdge(getVertIdx(LWR_FNT_LFT), getVertIdx(LWR_FNT_RGT));
		edges[1] = GridEdge(getVertIdx(LWR_BCK_LFT), getVertIdx(LWR_BCK_RGT));
		edges[2] = GridEdge(getVertIdx(UPR_BCK_LFT), getVertIdx(UPR_BCK_RGT));
		edges[3] = GridEdge(getVertIdx(UPR_FNT_LFT), getVertIdx(UPR_FNT_RGT));
		setMin();

	}

	void GridCell::getFaceIndices(FaceNumber faceNumber, size_t indices[4]) const {
		for (int i = 0; i < 4; i++) {
			indices[i] = _vertIndices[gFaceIdxLUT[faceNumber][i]];
		}
	}

	SearchableFace GridCell::getSearchableFace(FaceNumber faceNumber) const {
		size_t idx[4];
		getFaceIndices(faceNumber, idx);
		return SearchableFace(_id, faceNumber, idx);
	}

	void GridCell::getFaceTriIndices(FaceNumber faceNumber, size_t tri[2][3]) const {
		size_t verts[4];
		getFaceIndices(faceNumber, verts);

		tri[0][0] = verts[0];
		tri[0][1] = verts[1];
		tri[0][2] = verts[2];

		tri[1][0] = verts[0];
		tri[1][1] = verts[2];
		tri[1][2] = verts[3];
	}

	void GridCell::getFacePoints(const Grid& grid, FaceNumber faceNumber, const Vector3d* points[4]) const {
		size_t idx[4];
		getFaceIndices(faceNumber, idx);
		for (int i = 0; i < 4; i++) {
			points[i] = &grid.getVert(idx[i]).getPt();
		}
	}

	void GridCell::getFaceTriPoints(const GridBase& grid, FaceNumber faceNumber, const Vector3d* triPoints[2][3]) const {
		size_t triIdx[2][3];
		getFaceTriIndices(faceNumber, triIdx);
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 3; j++) {
				triPoints[i][j] = &grid.getVert(triIdx[i][j]).getPt();
			}
		}
	}

	bool GridCell::isPerpendicularBoundaryFace(const Grid& grid, FaceNumber faceNumber, TopolRef& clamp) const {
		size_t faceVerts[4];
		getFaceIndices(faceNumber, faceVerts);
		const Vector3d* facePts[4];
		getFacePoints(grid, faceNumber, facePts);
		Vector3d v0 = (*facePts[1]- *facePts[0]);
		Vector3d v1 = (*facePts[3] - *facePts[0]);
		Vector3d normal = v0.cross(v1).normalized();

		vector<TopolRef> parallelClamps;
		for (int i = 0; i < 4; i++) {
			const auto& vertClamp = grid.getVert(faceVerts[i]).getClamp();
			double dp = normal.dot(vertClamp.getVector());
			if (vertClamp.matches(CLAMP_PERPENDICULAR) && (1 - fabs(dp)) < 1.0e-6) {
				clamp = vertClamp;
				return true;
			} else if (vertClamp.matches(CLAMP_PARALLEL)) {
				parallelClamps.push_back(vertClamp);
			}
		}
		if (parallelClamps.size() == 2) {
			const Vector3d& v0 = parallelClamps[0].getVector();
			const Vector3d& v1 = parallelClamps[1].getVector();
			Vector3d perp = v0.cross(v1).normalized();
			double dp = normal.dot(perp);
			if ((1 - fabs(dp)) < SAME_DIST_TOL) {
				clamp = TopolRef::createPerpendicular(perp);
				return true;
			}
		}
		return false;
	}

	double GridCell::distToFace(const Grid& grid, FaceNumber faceNumber, const Vector3d& pt) const {
		const Vector3d* pts[4];
		getFacePoints(grid, faceNumber, pts);
		const Vector3d* tris[2][2][3] = {
			{
				{pts[0], pts[1], pts[2]},
				{pts[0], pts[2], pts[3]},
			},
			{
				{pts[1], pts[2], pts[3]},
				{pts[1], pts[2], pts[0]},
			},
		};
		double minDist = DBL_MAX;
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				Plane p(tris[i][j]);
				double d = fabs(distanceFromPlane(pt, p));
				if (d < SAME_DIST_TOL)
					return d;
				if (d < minDist)
					minDist = d;
			}
		}
		return minDist;
	}

	void GridCell::dump(std::ostream& out) const {
		string pad = "  ";
		out << "cell vert idx {\n";
		for (FaceNumber fn = BOTTOM; fn < FN_UNKNOWN; fn++) {
			out << "face indices (" << fn << ") : {";
			for (int i = 0; i < 4; i++) {
				size_t vidx = _vertIndices[gFaceIdxLUT[fn][i]];
				out << vidx;
				if (i < 3)
					out << ", ";
			}
			out << "}\n";

			out << "face verts (" << fn << ") : {";
			for (int i = 0; i < 4; i++) {
				size_t vidx = _vertIndices[gFaceIdxLUT[fn][i]];
				cout << vidx << ", ";
			}
			out << "}\n";
		}
		out << "}\n";

	}

}
