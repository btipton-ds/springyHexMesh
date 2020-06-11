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

#include <hm_polylineFitter.h>
#include <meshProcessor.h>
#include <hm_dump.h>

namespace HexahedralMesher {

	using namespace std;

	struct CPolylineFitter::FaceHit {
		inline FaceHit() 
		{}
		inline FaceHit(size_t cellIdx, CellVertPos startCorner, FaceNumber fn, const RayHit& hit, size_t plIdx)
			: _hit(hit)
			, _cellIdx(cellIdx)
			, _faceNumber(fn)
			, _plIdx(plIdx)
			, _startCorner(startCorner)
		{
			if (_startCorner == CVP_UNKNOWN) {
				throw "must set start corner";
			}
			if (_faceNumber == FN_UNKNOWN) {
				throw "must set face number";
			}
			if (_plIdx == stm1) {
				throw "must set polyline index.";
			}
		}

		RayHit _hit;
		FaceNumber _faceNumber = FN_UNKNOWN;
		CellVertPos _startCorner = CVP_UNKNOWN;
		size_t _cellIdx = stm1, _plIdx = stm1;
	};

	CPolylineFitter::CPolylineFitter(CMesher& mesher, size_t meshIdx, size_t polylineNumber)
		: _mesher(mesher)
		, _meshIdx(meshIdx)
		, _polylineNum(polylineNumber)
		, _grid(*mesher.getGrid())
		, _params(mesher.getParams())
		, _modelPtr(mesher.getModelPtr(meshIdx))
		, _polyline(_modelPtr->_polyLines[polylineNumber])
	{}

	size_t CPolylineFitter::doFit(set<size_t>& cellsToSplit) {
		size_t plIdx = 0;
		size_t numFitted = 0;
		size_t numSegs = _polyline.getVerts().size() - 1;
		size_t cornerIdx = findStartingCornerIndex();
		size_t count = 0;
		while (cornerIdx != stm1) {
			if (_polylineNum == 1 && count == 3) {
				int dbgBreak = 1;
			}
			map<size_t, vector<FaceHit>> cellFaceHits;
			if (!findCellFaceHits(cornerIdx, cellFaceHits, cellsToSplit)) {
				// TODO _usually_ exits when it hits the end of the line
				// Need a deterministic test for that. For now just break.
				break;
			}
			if (!fitCells(cellFaceHits, cornerIdx, plIdx, cellsToSplit)) {
				break;
			}
			const GridVert& cornerVert = _grid.getVert(cornerIdx);
			if (!cornerVert.getClamp().matches(CLAMP_EDGE | CLAMP_VERT) && plIdx > 0) {
				break;
			}

			count++;
			numFitted++;
		}
		addPolylineEndToClamped();
		getClampedCells(cellsToSplit);
		return numFitted;
	}

	size_t CPolylineFitter::findStartingCornerIndex() const {
		const Vector3d& pt = _modelPtr->getVert(_polyline.getVerts().front())._pt;
		BoundingBox bb(pt);
		vector<size_t> vertIndices = _grid.findVerts(bb);
		double minDist = DBL_MAX;
		size_t bestVert = stm1;
		for (size_t vertIdx : vertIndices) {
			const Vector3d& pt1 = _grid.getVert(vertIdx).getPt();
			double d = (pt1 - pt).squaredNorm();
			if (d < minDist) {
				minDist = d;
				bestVert = vertIdx;
			}
		}
		return bestVert;
	}

	bool CPolylineFitter::findCellFaceHits(size_t startVertIdx, map<size_t, vector<FaceHit>>& hits,
		set<size_t>& cellsToSplit) const {
		const GridVert& startVert = _grid.getVert(startVertIdx);
		const Vector3d& cornerPt = startVert.getPt();

		vector<size_t> allIndices;
		const vector<size_t>& cellIndices = startVert.getCellIndices();
		for (size_t cellIdx : cellIndices) {
			const GridCell& cell = _grid.getCell(cellIdx);
			int numClamped = cell.getNumClamped(_grid, CLAMP_EDGE) + cell.getNumClamped(_grid, CLAMP_VERT);
			if (numClamped < 2) {
				allIndices.push_back(cellIdx);
			}
		}

		if (allIndices.empty()) {
			return false;
		}

		for (size_t cellIdx : allIndices) {
			const GridCell& cell = _grid.getCell(cellIdx);
			const GridVert& vert = _grid.getVert(startVertIdx);

			CellVertPos corner = cell.getVertsPos(startVertIdx);
			vector<FaceHit> allHits;
			if (findPierces(cellIdx, corner, allHits) > 0) {
				cellsToSplit.insert(cellIdx);
				vector<FaceHit> usefulHits;
				for (const FaceHit& fh : allHits) {
					if (tolerantEquals(fh._hit.hitPt, cornerPt))
						continue;
					usefulHits.push_back(fh);
				}
				if (usefulHits.size() > 0) {
					hits.insert(make_pair(cellIdx, usefulHits));
				}
			}
		}

		return hits.size();
	}

	size_t CPolylineFitter::findPierces(size_t cellIdx, CellVertPos corner, vector<FaceHit>& hits) const {
		const GridCell& cell = _grid.getCell(cellIdx);

		Vector3dSet hitPts;
		for (FaceNumber fn = BOTTOM; fn < FN_UNKNOWN; fn++) {
			const Vector3d* triPts[2][3];
			cell.getFaceTriPoints(_grid, fn, triPts);
			for (int i = 0; i < 2; i++) {
				RayHit hit;
				size_t hitPlIdx = polylineIntersectsTri(0, triPts[i], hit);
				if (hitPlIdx != stm1) {
					if (hitPts.count(hit.hitPt) == 0) {
						hitPts.insert(hit.hitPt);
						hits.push_back(FaceHit(cellIdx, corner, fn, hit, hitPlIdx));
					}
					break;
				}
			}
		}

		return hits.size();
	}

	size_t CPolylineFitter::polylineIntersectsTri(size_t startIdx, const Vector3d* tri[3], RayHit& hit) const {
		const auto& verts = _polyline.getVerts();
		for (size_t plIdx = startIdx; plIdx < verts.size() - 1; plIdx++) {
			LineSegment seg(_modelPtr->getVert(verts[plIdx])._pt, _modelPtr->getVert(verts[plIdx + 1])._pt);
			if (intersectLineSegTri(seg, tri, hit))
				return plIdx;
		}
		return stm1;
	}

	bool CPolylineFitter::fitCells(const map<size_t, vector<FaceHit>>& cellFaceHits, size_t& cornerIdx, size_t& plIdx,
		set<size_t>& cellsToSplit) {
		for (const auto& iter : cellFaceHits) {
			size_t cellIdx = iter.first;
			const GridCell& cell = _grid.getCell(cellIdx);
			CellVertPos cornerPos = cell.getVertsPos(cornerIdx);
			if (cornerPos == CVP_UNKNOWN)
				throw "Should always get cells containing this vertex.";

			const vector<FaceHit>& faceHits = iter.second;
			CellVertPos clampCorner = CVP_UNKNOWN;
			FaceHit clampFaceHit;
			double minDist = DBL_MAX;
			for (const FaceHit& fh : faceHits) {
				CellVertPos p = findClosestUnclampedCorner(cellIdx, cornerPos, fh);
				if (p != CVP_UNKNOWN) {
					const GridVert& vert = cell.getVert(_grid, p);
					double d = (fh._hit.hitPt - vert.getPt()).squaredNorm();
					if (d < minDist) {
						minDist = d;
						clampCorner = p;
						clampFaceHit = fh;
					}
				}
			}
			if (clampCorner != CVP_UNKNOWN) {
				int numClamped = cell.getNumClamped(_grid, CLAMP_EDGE) + cell.getNumClamped(_grid, CLAMP_VERT);
				if (numClamped < 2) {
					putUnclampedCornerOnPolyline(cellIdx, clampCorner, clampFaceHit);
				} else {
					numClamped = cell.getNumClamped(_grid, CLAMP_EDGE) + cell.getNumClamped(_grid, CLAMP_VERT);
				}

				cornerIdx = cell.getVertIdx(clampCorner);
				plIdx = clampFaceHit._plIdx;
				return true;
			}
		}
		return false;
	}

	CellVertPos CPolylineFitter::findClosestUnclampedCorner(size_t cellIdx, CellVertPos ignorePos, const FaceHit& faceHit) const {
		CellVertPos faceCornerPos[4];
		GridCell::getFaceCellPos(faceHit._faceNumber, faceCornerPos);
		const GridCell& cell = _grid.getCell(cellIdx);

		size_t faceIndices[4];
		for (int i = 0; i < 4; i++) {
			if (faceCornerPos[i] == ignorePos)
				faceIndices[i] = stm1;
			else
				faceIndices[i] = cell.getVertIdx(faceCornerPos[i]);
		}

		CellVertPos corner = faceHit._startCorner;
		CellVertPos oppCorner = GridCell::getOppCorner(corner);
		CellVertPos nearEdgeEnds[3];
		GridCell::getAdjacentEdgeEnds(corner, nearEdgeEnds);

		int bestIdx = -1;
		double minVertEnergy = DBL_MAX;
		for (int i = 0; i < 4; i++) {
			if (faceIndices[i] == stm1)
				continue;
			size_t vertIdx = faceIndices[i];
			const GridVert& vert = _grid.getVert(vertIdx);
			TopolRef originalClamp = vert.getClamp();
			Vector3d originalPt = vert.getPt();
			const Vector3d& clampPt = faceHit._hit.hitPt;
			Vector3d v = vert.getPt() - clampPt;
			switch (originalClamp.getClampType()) {
			case CLAMP_FIXED:
			case CLAMP_VERT:
			case CLAMP_EDGE:
			case CLAMP_TRI:
				continue;
			case CLAMP_PERPENDICULAR:
				if (fabs(v.dot(originalClamp.getVector())) > SAME_DIST_TOL)
					continue;
				break;
			case CLAMP_PARALLEL:
				if ((v - originalClamp.getVector() * originalClamp.getVector().dot(v)).squaredNorm() > SAME_DIST_TOL_SQR)
					continue;
				break;
			}
			CellVertPos vertPos = faceCornerPos[i];
			if (vertPos == corner)
				continue;

			double weight = 1.0;
			if (vertPos == nearEdgeEnds[0] || vertPos == nearEdgeEnds[1] || vertPos == nearEdgeEnds[2])
				weight = 1.0; // prefer adjacent corners
			else if (vertPos == oppCorner)
				continue; // Avoid the opposite corner
			weight *= weight;

			double vertEnergy = weight * _grid.calcVertexOrthoEnergyAtPos(vertIdx, clampPt);

			if (vertEnergy < minVertEnergy) {
				minVertEnergy = vertEnergy;
				bestIdx = i;
			}

		}

		return cell.getVertsPos(faceIndices[bestIdx]);
	}

	void CPolylineFitter::putUnclampedCornerOnPolyline(size_t cellIdx, CellVertPos corner, const FaceHit& faceHit) {
		GridCell& cell = _grid.getCell(cellIdx);
		size_t vertIdx = cell.getVertIdx(corner);
		GridVert& vert = _grid.getVert(vertIdx);
		const TopolRef& clamp = vert.getClamp();

		const Vector3d& curPt = vert.getPt();
		Vector3d clampPt = faceHit._hit.hitPt;
		switch (clamp.getClampType()) {
		case CLAMP_PERPENDICULAR: {
			// Lock point on the plane perpendicular to the current point
			Vector3d v = clampPt - curPt;
			v = v - clamp.getVector() * clamp.getVector().dot(v);
			clampPt = curPt + v;
			break;
		}
		case CLAMP_PARALLEL: {
			// Lock point on the line through the current point
			Vector3d v = clampPt - curPt;
			clampPt = curPt + clamp.getVector() * clamp.getVector().dot(v);
			break;
		}
		default:
			break;
		}
		const auto& pl = _modelPtr->_polyLines[_polylineNum];
		size_t plIdx;
		double d, t;
		if (!pl.findClosestPointOnPolyline(*_modelPtr, clampPt, plIdx, d, t)) {
			cout << "Is not clamped to the polyline.\n";
		}
		if (!_grid.setVertPos(vertIdx, clampPt)) {
			cout << "Error, fused a point. putUnclampedCornerOnPolyline\n";
		}

		if (vert.getClampType() == CLAMP_NONE) {
			vert.setClamp(_grid, TopolRef::createPolylineRef(_meshIdx, _polylineNum, faceHit._plIdx));
		} else
			vert.setClamp(_grid, TopolRef::createFixed()); // This case fixes a boundary vertext to a crossing edge.
		if (cell.getNumClamped(_grid) < 2) {
			cout << "Expected 2 clamped verts and got " << cell.getNumClamped(_grid) << "\n";
		}
		_clampedVertIndices.push_back(vertIdx);
	}

	void CPolylineFitter::addPolylineEndToClamped() {
		const auto& pt = _modelPtr->getVert(_polyline.getVerts().back())._pt;
		vector<size_t> nearPts;
		if (_grid.findVerts(BoundingBox(pt), nearPts) == 1) {
			_clampedVertIndices.push_back(nearPts.front());
		}

	}

	void CPolylineFitter::getClampedCells(set<size_t>& cellsToSplit) {
		for (size_t vertIdx : _clampedVertIndices) {
			const auto& vert = _grid.getVert(vertIdx);
			const auto& cellIndices = vert.getCellIndices();
			cellsToSplit.insert(cellIndices.begin(), cellIndices.end());
		}
	}

}