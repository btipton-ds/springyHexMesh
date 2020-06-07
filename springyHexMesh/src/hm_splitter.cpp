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

#include <hm_splitter.h>

#include <iostream>

#include <tm_math.h>
#include <hm_gridVert.h>
#include <hm_gridCell.h>
#include <hm_grid.h>
#include <hm_splitter.h>
#include <meshProcessor.h>

namespace HexahedralMesher {

	using namespace std;

	struct CSplitter::SplitCellDiagonallyRec {
		size_t _cellIdx;
		FaceNumber _faceNumber;
		CellVertPos _corners[2];
	};

	struct CSplitter::SplitSourceRec {
		SplitSourceRec(SplitType type, CSplitter& splitter, size_t cellId);
		void addChild(size_t cellId);
		size_t findVert(const Vector3d& pt) const;

		Grid& _grid;
		CSplitter& _splitter;
		SplitType _type;
		GridCell _cell;

		size_t _triIdx[2][2][3];
		FaceNumber _faceNumber;
		CellVertPos _corner0, _corner1;
		std::set<size_t> _verts;
	};

	CSplitter::CSplitter(Grid& grid)
		: _grid(grid)
		, _numInitialVerts(grid.numVerts())
	{
		clear();
	}

	CSplitter::~CSplitter() {
		postSplit();
	}

	inline void CSplitter::setPoint(size_t vertIdx, const Vector3d& pt) {
		_changedPointMap[vertIdx] = pt;
	}

	void CSplitter::splitAll() {
		clear();

		_grid.iterateCells([&](size_t cellIdx)->bool {
			splitCellFull(cellIdx);
			return true;
		});

		postSplit();
	}

	vector<size_t> CSplitter::getClampedCells() const {
		set<size_t> clampedCells;

		for (size_t vertIdx : _clampedVerts) {
			const vector<size_t>& cellIndices = getVert(vertIdx).getCellIndices();
			clampedCells.insert(cellIndices.begin(), cellIndices.end());
		}
		vector<size_t> result;
		result.insert(result.end(), clampedCells.begin(), clampedCells.end());
		return result;
	}

	void CSplitter::splitCellFull(size_t cellIdx) {
		if (!_grid.cellExists(cellIdx))
			return;

		splitCellFullInit(cellIdx);
		SplitSourceRec& splitRec = createOctSplit(cellIdx);

		addSubCellLwrFntLft(splitRec);
		addSubCellLwrFntRgt(splitRec);
		addSubCellLwrBckLft(splitRec);
		addSubCellLwrBckRgt(splitRec);

		addSubCellUprFntLft(splitRec);
		addSubCellUprFntRgt(splitRec);
		addSubCellUprBckLft(splitRec);
		addSubCellUprBckRgt(splitRec);
	}

	void CSplitter::clear() {
		_cellsToClamp.clear();
		_clampedVerts.clear();
		_workCell = GridCell();
		_newCells.clear();
		_cellsToClamp.clear();
		_finalEdgeSet.clear();
		_changedPointMap.clear();
	}

	void CSplitter::postSplit() {
		buildEdgeSet();
		for (auto& splitRec : _cellsToClamp)
			clampToBoundaries(splitRec);

		for (auto& splitRec : _cellsToClamp) {
			switch (splitRec._type) {
			case SPLIT_OCT:
				applyOctSplitClamps(splitRec);
				break;
			case SPLIT_TRI_PRISM:
				applyTriPrismSplitClamps(splitRec);
				break;
			}

			clampToAdjacentCells(splitRec);
		}
		_finalEdgeSet.clear();
		_cellsToClamp.clear();

		// Apply all of the moves after clamping. This keeps the source cells in synch with their subcells.
		// That allows us to use geometric matching without anything moving between clamps.
		for (const auto& iter : _changedPointMap) {
			size_t vertIdx = iter.first;
			getVert(vertIdx).setPoint(iter.second);
		}

		fixBrokenLinks();

		// Don't clear all. The caller will want access to the outputs
		_changedPointMap.clear();
	}

	void CSplitter::fixBrokenLinks() {
		_grid.iterateVerts([&](size_t vertIdx)->bool {
			auto& vert = getVert(vertIdx);
			auto& clamp = vert.getClamp();
			switch (vert.getClampType()) {
				default:
					break;
				case CLAMP_CELL_EDGE_CENTER: {
					if (_grid.cellExists(clamp.getCellIdx()))
						break;
					vert.setClamp(_grid, TopolRef::createNone());
					if (clampVertToGridBoundary(vertIdx))
						return true;
					break;
				}
			}
			return true;
		}, 1);
	}


	void CSplitter::splitCells(set<size_t> cells) {
		clear();

		map<GridEdge, vector<SplitCellDiagonallyRec>> cellPairsToSplit;
		for (size_t cellIdx : cells) {
			splitCellDiagonally(cellIdx, cellPairsToSplit);
		}

		for (auto iter : cellPairsToSplit) {
			const auto& cellRecs = iter.second;
			for (const auto& rec : cellRecs) {
				splitCellFaceDiagonally(rec._cellIdx, rec._faceNumber, rec._corners[0], rec._corners[1]);
			}
		}

		for (size_t cellIdx : cells) {
			splitCellFull(cellIdx);
		}

		postSplit();
	}

	bool CSplitter::splitCellDiagonally(size_t cellIdx, map<GridEdge, vector<SplitCellDiagonallyRec>>& cellPairsToSplit) {
		if (!_grid.cellExists(cellIdx))
			return false;
		auto& cell = getCell(cellIdx);

		for (FaceNumber fn = BOTTOM; fn < FN_UNKNOWN; fn++) {
			CellVertPos facePos[4];
			size_t faceVertIndices[4];

			GridCell::getFaceCellPos(fn, facePos);
			cell.getFaceIndices(fn, faceVertIndices);

			bool clamps[4];
			int numFaceClamps = 0;
			for (int i = 0; i < 4; i++) {
				const GridVert& vert = getVert(faceVertIndices[i]);
				ClampType ct = vert.getClampType();
				clamps[i] = (ct == CLAMP_EDGE || ct == CLAMP_VERT);
				if (clamps[i])
					numFaceClamps++;
			}
			if (numFaceClamps != 2)
				continue;

			SplitCellDiagonallyRec rec;
			GridEdge edge;
			rec._cellIdx = cellIdx;
			rec._faceNumber = fn;
			if (clamps[0] && clamps[2]) {
				edge = GridEdge(cell.getVertIdx(facePos[0]), cell.getVertIdx(facePos[2]));
				rec._corners[0] = facePos[0];
				rec._corners[1] = facePos[2];
			}
			else if (clamps[1] && clamps[3]) {
				edge = GridEdge(cell.getVertIdx(facePos[1]), cell.getVertIdx(facePos[3]));
				rec._corners[0] = facePos[1];
				rec._corners[1] = facePos[3];
			}
			else
				continue;
			auto iter = cellPairsToSplit.find(edge);
			if (iter == cellPairsToSplit.end())
				iter = cellPairsToSplit.insert(make_pair(edge, vector<SplitCellDiagonallyRec>())).first;
			iter->second.push_back(rec);
		}

		return false;
	}

	inline size_t CSplitter::add(const Vector3d& pt) {
		return _grid.add(pt);
	}

	size_t CSplitter::addCell(GridCell& cell, SplitSourceRec& splitRec) {
		size_t cellIdx = _grid.addCell(cell);
		splitRec.addChild(cellIdx);
		_newCells.push_back(cellIdx);
		if (!getCell(cellIdx).verify(_grid, true)) {
			getCell(cellIdx).verify(_grid, true);
			cout << "Divided cell failed verification.\n";
		}
		return cellIdx;
	}
	
	void CSplitter::splitCellFullInit(size_t cellIdx) {
		_workCell = getCell(cellIdx);
		_center = add(_workCell.calcCentroid(_grid));
		for (int i = 0; i < 12; i++) {
			GridEdge edge = _workCell.getEdge(i);
			_edgeCenters[edge] = add(edge.calcCenter(_grid));
		}
		for (FaceNumber fn = BOTTOM; fn < FN_UNKNOWN; fn++) {
			_faceCenters[fn] = add(_workCell.calcFaceCentroid(_grid, fn));
		}
	}

#define SET_OCT_REST_EDGE_LEN \
for (int i = 0; i < 12; i++) \
	cell.setRestEdgeLength(i, _workCell.getRestEdgeLength(i) / 2)

	size_t CSplitter::addSubCellLwrFntLft(SplitSourceRec& splitRec) {
		GridCell cell;
		SET_OCT_REST_EDGE_LEN;

		cell.setVertIdx(LWR_FNT_LFT, corn(LWR_FNT_LFT));
		cell.setVertIdx(LWR_FNT_RGT, edge(LWR_FNT_LFT, LWR_FNT_RGT));
		cell.setVertIdx(LWR_BCK_RGT, face(BOTTOM));
		cell.setVertIdx(LWR_BCK_LFT, edge(LWR_FNT_LFT, LWR_BCK_LFT));

		cell.setVertIdx(UPR_FNT_LFT, edge(LWR_FNT_LFT, UPR_FNT_LFT));
		cell.setVertIdx(UPR_FNT_RGT, face(FRONT));
		cell.setVertIdx(UPR_BCK_RGT, _center);
		cell.setVertIdx(UPR_BCK_LFT, face(LEFT));

		return addCell(cell, splitRec);
	}

	size_t CSplitter::addSubCellLwrFntRgt(SplitSourceRec& splitRec) {
		GridCell cell;
		SET_OCT_REST_EDGE_LEN;

		cell.setVertIdx(LWR_FNT_LFT, edge(LWR_FNT_LFT, LWR_FNT_RGT));
		cell.setVertIdx(LWR_FNT_RGT, corn(LWR_FNT_RGT));
		cell.setVertIdx(LWR_BCK_RGT, edge(LWR_FNT_RGT, LWR_BCK_RGT));
		cell.setVertIdx(LWR_BCK_LFT, face(BOTTOM));

		cell.setVertIdx(UPR_FNT_LFT, face(FRONT));
		cell.setVertIdx(UPR_FNT_RGT, edge(LWR_FNT_RGT, UPR_FNT_RGT));
		cell.setVertIdx(UPR_BCK_RGT, face(RIGHT));
		cell.setVertIdx(UPR_BCK_LFT, _center);

		return addCell(cell, splitRec);
	}

	size_t CSplitter::addSubCellLwrBckLft(SplitSourceRec& splitRec) {
		GridCell cell;

		SET_OCT_REST_EDGE_LEN;
		cell.setVertIdx(LWR_FNT_LFT, edge(LWR_FNT_LFT, LWR_BCK_LFT));
		cell.setVertIdx(LWR_FNT_RGT, face(BOTTOM));
		cell.setVertIdx(LWR_BCK_RGT, edge(LWR_BCK_LFT, LWR_BCK_RGT));
		cell.setVertIdx(LWR_BCK_LFT, corn(LWR_BCK_LFT));

		cell.setVertIdx(UPR_FNT_LFT, face(LEFT));
		cell.setVertIdx(UPR_FNT_RGT, _center);
		cell.setVertIdx(UPR_BCK_RGT, face(BACK));
		cell.setVertIdx(UPR_BCK_LFT, edge(LWR_BCK_LFT, UPR_BCK_LFT));

		return addCell(cell, splitRec);
	}

	size_t CSplitter::addSubCellLwrBckRgt(SplitSourceRec& splitRec) {
		GridCell cell;

		SET_OCT_REST_EDGE_LEN;
		cell.setVertIdx(LWR_FNT_LFT, face(BOTTOM));
		cell.setVertIdx(LWR_FNT_RGT, edge(LWR_FNT_RGT, LWR_BCK_RGT));
		cell.setVertIdx(LWR_BCK_RGT, corn(LWR_BCK_RGT));
		cell.setVertIdx(LWR_BCK_LFT, edge(LWR_BCK_LFT, LWR_BCK_RGT));

		cell.setVertIdx(UPR_FNT_LFT, _center);
		cell.setVertIdx(UPR_FNT_RGT, face(RIGHT));
		cell.setVertIdx(UPR_BCK_RGT, edge(LWR_BCK_RGT, UPR_BCK_RGT));
		cell.setVertIdx(UPR_BCK_LFT, face(BACK));

		return addCell(cell, splitRec);
	}


	size_t CSplitter::addSubCellUprFntLft(SplitSourceRec& splitRec) {
		GridCell cell;

		SET_OCT_REST_EDGE_LEN;
		cell.setVertIdx(LWR_FNT_LFT, edge(LWR_FNT_LFT, UPR_FNT_LFT));
		cell.setVertIdx(LWR_FNT_RGT, face(FRONT));
		cell.setVertIdx(LWR_BCK_RGT, _center);
		cell.setVertIdx(LWR_BCK_LFT, face(LEFT));

		cell.setVertIdx(UPR_FNT_LFT, corn(UPR_FNT_LFT));
		cell.setVertIdx(UPR_FNT_RGT, edge(UPR_FNT_LFT, UPR_FNT_RGT));
		cell.setVertIdx(UPR_BCK_RGT, face(TOP));
		cell.setVertIdx(UPR_BCK_LFT, edge(UPR_FNT_LFT, UPR_BCK_LFT));

		return addCell(cell, splitRec);
	}

	size_t CSplitter::addSubCellUprFntRgt(SplitSourceRec& splitRec) {
		GridCell cell;

		SET_OCT_REST_EDGE_LEN;
		cell.setVertIdx(LWR_FNT_LFT, face(FRONT));
		cell.setVertIdx(LWR_FNT_RGT, edge(LWR_FNT_RGT, UPR_FNT_RGT));
		cell.setVertIdx(LWR_BCK_RGT, face(RIGHT));
		cell.setVertIdx(LWR_BCK_LFT, _center);

		cell.setVertIdx(UPR_FNT_LFT, edge(UPR_FNT_LFT, UPR_FNT_RGT));
		cell.setVertIdx(UPR_FNT_RGT, corn(UPR_FNT_RGT));
		cell.setVertIdx(UPR_BCK_RGT, edge(UPR_FNT_RGT, UPR_BCK_RGT));
		cell.setVertIdx(UPR_BCK_LFT, face(TOP));

		return addCell(cell, splitRec);
	}

	size_t CSplitter::addSubCellUprBckLft(SplitSourceRec& splitRec) {
		GridCell cell;

		SET_OCT_REST_EDGE_LEN;
		cell.setVertIdx(LWR_FNT_LFT, face(LEFT));
		cell.setVertIdx(LWR_FNT_RGT, _center);
		cell.setVertIdx(LWR_BCK_RGT, face(BACK));
		cell.setVertIdx(LWR_BCK_LFT, edge(LWR_BCK_LFT, UPR_BCK_LFT));

		cell.setVertIdx(UPR_FNT_LFT, edge(UPR_FNT_LFT, UPR_BCK_LFT));
		cell.setVertIdx(UPR_FNT_RGT, face(TOP));
		cell.setVertIdx(UPR_BCK_RGT, edge(UPR_BCK_LFT, UPR_BCK_RGT));
		cell.setVertIdx(UPR_BCK_LFT, corn(UPR_BCK_LFT));

		return addCell(cell, splitRec);
	}

	size_t CSplitter::addSubCellUprBckRgt(SplitSourceRec& splitRec) {
		GridCell cell;

		SET_OCT_REST_EDGE_LEN;
		cell.setVertIdx(LWR_FNT_LFT, _center);
		cell.setVertIdx(LWR_FNT_RGT, face(RIGHT));
		cell.setVertIdx(LWR_BCK_RGT, edge(LWR_BCK_RGT, UPR_BCK_RGT));
		cell.setVertIdx(LWR_BCK_LFT, face(BACK));

		cell.setVertIdx(UPR_FNT_LFT, face(TOP));
		cell.setVertIdx(UPR_FNT_RGT, edge(UPR_FNT_RGT, UPR_BCK_RGT));
		cell.setVertIdx(UPR_BCK_RGT, corn(UPR_BCK_RGT));
		cell.setVertIdx(UPR_BCK_LFT, edge(UPR_BCK_LFT, UPR_BCK_RGT));

		return addCell(cell, splitRec);
	}

	void CSplitter::splitCellFaceDiagonally(size_t cellIdx, FaceNumber faceNumber, CellVertPos corner0, CellVertPos corner1) {
		GridCell baseCell = getCell(cellIdx);

		FaceNumber opFaceNumber = GridCell::getOppFace(faceNumber);

		SplitSourceRec& splitRec = createTriPrismSplit(cellIdx, faceNumber, corner0, corner1);
		size_t numInitialCells = _grid.numCells();

		size_t cornerIdx0 = baseCell.getVertIdx(corner0);
		size_t cornerIdx1 = baseCell.getVertIdx(corner1);
		const auto& baseCorner0 = getVert(cornerIdx0);
		const auto& baseCorner1 = getVert(cornerIdx1);
		Vector3d baseFaceCtr(0, 0, 0);
		baseFaceCtr += baseCorner0.getPt();
		baseFaceCtr += baseCorner1.getPt();
		baseFaceCtr /= 2;

		//splitInto traingles;
		size_t faceIdx[2][4];
		size_t cornerIdx[2];
		baseCell.getFaceIndices(faceNumber, faceIdx[0]);
		baseCell.getFaceIndices(opFaceNumber, faceIdx[1]);

		swap(faceIdx[1][1], faceIdx[1][3]); // invert the normal of the other face
		cornerIdx[0] = baseCell.getVertIdx(corner0);
		// This function needs a clearer name: Get the edge connected vertex on the opposite face;
		cornerIdx[1] = baseCell.getOppositeEdgeEndVertIdx(faceNumber, corner0);
		int offset[2][3] = {
			{0, 0, 0},
			{0, 1, 1}
		};

		for (int i = 0; i < 2; i++) {
			int start;

			start = -1;
			for (int j = 0; j < 4; j++) {
				if (faceIdx[i][j] == cornerIdx[i]) {
					start = j;
					break;
				}
			}

			for (int j = 0; j < 3; j++) {
				int idx0 = (start + j + offset[0][j]) % 4;
				int idx1 = (start + j + offset[1][j]) % 4;
				splitRec._triIdx[i][0][j] = faceIdx[i][idx0];
				splitRec._triIdx[i][1][j] = faceIdx[i][idx1];
			}
		}

		addTriangluarPrism(splitRec._triIdx[0][0], splitRec._triIdx[1][0], splitRec);
		addTriangluarPrism(splitRec._triIdx[0][1], splitRec._triIdx[1][1], splitRec);
	}

	void CSplitter::addTriangluarPrism(const size_t tri0[3], const size_t tri1[3], SplitSourceRec& splitRec) {
		size_t triVertIdx[2][3];
		for (int i = 0; i < 3; i++) {
			triVertIdx[0][i] = tri0[i];
			triVertIdx[1][i] = tri1[i];
		}

		Vector3d triPts[2][3];
		for (int triNum = 0; triNum < 2; triNum++) {
			for (int i = 0; i < 3; i++) {
				const auto& vert = getVert(triVertIdx[triNum][i]);
				triPts[triNum][i] = vert.getPt();
			}
		}

		size_t ctrIdx[2];
		size_t edgeCtrIdx[2][3];
		for (int triNum = 0; triNum < 2; triNum++) {
			for (int i = 0; i < 3; i++) {
				Vector3d pt;
				pt = (triPts[triNum][i] + triPts[triNum][(i + 1) % 3]) / 2;
				edgeCtrIdx[triNum][i] = add(pt);
			}
			Vector3d ctr = triangleCentroid(triPts[triNum]);
			ctrIdx[triNum] = add(ctr);
		}


		size_t quad[2][3][4];
		for (int i = 0; i < 2; i++) {
			quad[i][0][0] = triVertIdx[i][0];
			quad[i][0][1] = edgeCtrIdx[i][0];
			quad[i][0][2] = ctrIdx[i];
			quad[i][0][3] = edgeCtrIdx[i][2];

			quad[i][1][0] = edgeCtrIdx[i][0];
			quad[i][1][1] = triVertIdx[i][1];
			quad[i][1][2] = edgeCtrIdx[i][1];
			quad[i][1][3] = ctrIdx[i];

			quad[i][2][0] = ctrIdx[i];
			quad[i][2][1] = edgeCtrIdx[i][1];
			quad[i][2][2] = triVertIdx[i][2];
			quad[i][2][3] = edgeCtrIdx[i][2];
		}
		for (int i = 0; i < 3; i++) {
			addQuadPrism(quad[0][i], quad[1][i], splitRec);
		}
	}

	size_t CSplitter::addQuad(const size_t frontFace[4], const size_t backFace[4], SplitSourceRec& splitRec) {
		GridCell newCell;

		newCell.setVertIdx(LWR_FNT_LFT, frontFace[0]);
		newCell.setVertIdx(LWR_FNT_RGT, frontFace[1]);
		newCell.setVertIdx(UPR_FNT_RGT, frontFace[2]);
		newCell.setVertIdx(UPR_FNT_LFT, frontFace[3]);

		newCell.setVertIdx(LWR_BCK_LFT, backFace[0]);
		newCell.setVertIdx(LWR_BCK_RGT, backFace[1]);
		newCell.setVertIdx(UPR_BCK_RGT, backFace[2]);
		newCell.setVertIdx(UPR_BCK_LFT, backFace[3]);

		newCell.setDefaultRestEdgeLengths(_grid);

		size_t cellIdx = addCell(newCell, splitRec);
#if 1
		const auto& cell = getCell(cellIdx);
		if (!cell.verify(_grid, true)) {
			cell.verify(_grid, true);
			throw "Bad cell";
		}
#endif
		return cellIdx;
	}

	void CSplitter::addQuadPrism(const size_t frontFace[4], const size_t backFace[4], SplitSourceRec& splitRec) {
		size_t faces[3][4];
		for (int i = 0; i < 4; i++) {
			faces[0][i] = frontFace[i];
			faces[2][i] = backFace[i];
		}

		for (int i = 0; i < 4; i++) {
			const Vector3d& frontPt = _grid.getVert(faces[0][i]).getPt();
			const Vector3d& backPt = _grid.getVert(faces[2][i]).getPt();
			Vector3d midPt = (frontPt + backPt) * 0.5;
			Vector3d dir = (frontPt - backPt).normalized();
			faces[1][i] = add(midPt);
		}

		addQuad(faces[0], faces[1], splitRec);
		addQuad(faces[1], faces[2], splitRec);
	}

	void CSplitter::clampToEdgeBoundaries(SplitSourceRec& splitRec) {
		const auto& srcCell = splitRec._cell;
		for (int i = 0; i < 12; i++) {
			GridEdge edge = srcCell.getEdge(i);
			TopolRef clamp;
			if (edge.isBoundary(_grid, clamp)) {
				auto& verts = splitRec._verts;
				for (auto iter = verts.begin(); iter != verts.end(); iter++) {
					size_t vertIdx = *iter;
					if (clampVertToDeletedBoundaryEdge(vertIdx, edge)) {
						_clampedVerts.push_back(vertIdx);
						verts.erase(iter--);
						break;
					}
				}
			}
		}
	}

	bool CSplitter::clampVertToDeletedBoundaryEdge(size_t vertIdx, const GridEdge& edge) {
		if (isClamped(vertIdx))
			return false;

		auto& vert = getVert(vertIdx);
		TopolRef clamp;
		if (edge.isBoundary(_grid, clamp)) {
			double t;
			double d = edge.distToPoint(_grid, vert.getPt(), t);
			if (d < SAME_DIST_TOL && 0 <= t && t < 1) {
				vert.setClamp(_grid, clamp);
				return true;
			}
		}
		return false;
	}

	void CSplitter::clampToFaceBoundaries(SplitSourceRec& splitRec) {
		const auto& srcCell = splitRec._cell;
		for (FaceNumber fn = BOTTOM; fn < FN_UNKNOWN; fn++) {
			TopolRef clamp;
			if (srcCell.isPerpendicularBoundaryFace(_grid, fn, clamp)) {
				const Vector3d* triPts[2][3];
				srcCell.getFaceTriPoints(_grid, fn, triPts);
				Plane facePlane(triPts[0]);
				auto& verts = splitRec._verts;
				for (auto iter = verts.begin(); iter != verts.end(); iter++) {
					size_t vertIdx = *iter;
					auto& vert = getVert(vertIdx);
					if (isClamped(vertIdx))
						continue;

					double dist = facePlane.distanceToPoint(vert.getPt());
					if (dist < SAME_DIST_TOL) {
						if (clampVertToCellEdgeMidPoints(vertIdx)) {
							verts.erase(iter--);
						} else {
							vert.setClamp(_grid, clamp);
							verts.erase(iter--);
							_clampedVerts.push_back(vertIdx);
						}
					}
				}
			}
		}
	}

	void CSplitter::buildEdgeSet() {
		_finalEdgeSet.clear();
		_grid.iterateCells([&](size_t cellId)->bool {
			const auto& cell = getCell(cellId);
			for (int i =0; i < 12; i++)
				_finalEdgeSet.insert(cell.getEdge(i));
			return true;
		});
	}

	bool CSplitter::clampVertToCellEdgeMidPoint(size_t vertIdx, const GridEdge& edge) {
		auto& vert = getVert(vertIdx);
		const auto& pt = vert.getPt();
		Vector3d midPt = edge.calcCenter(_grid);
		double d = (pt - midPt).squaredNorm();
		if (d < SAME_DIST_TOL_SQR) {
			setPoint(vertIdx, midPt);
			vert.setClamp(_grid, TopolRef::createGridEdgeMidPtRef(edge));
			_clampedVerts.push_back(vertIdx);
			return true;
		}
		return false;
	}

	bool CSplitter::clampVertToCellEdgeMidPoints(size_t vertIdx) {
		for (const auto& edge : _finalEdgeSet) {
			if (clampVertToCellEdgeMidPoint(vertIdx, edge))
				return true;
		}
		return false;
	}

	void CSplitter::clampToBoundaries(SplitSourceRec& splitRec) {
		clampToEdgeBoundaries(splitRec);
		clampToFaceBoundaries(splitRec);
	}

	void CSplitter::clampToAdjacentCells(SplitSourceRec& splitRec) {
		auto& cell = splitRec._cell;

		for (int edgeNum = 0; edgeNum < 12; edgeNum++) {
			clampToAdjacentCellEdge(splitRec, edgeNum);
		}

		for (FaceNumber faceNum = BOTTOM; faceNum < FN_UNKNOWN; faceNum++) {
			clampToAdjacentCellFace(splitRec, faceNum);
		}
	}

	void CSplitter::clampToAdjacentCellEdge(SplitSourceRec& splitRec, int edgeNum) {
		const auto& srcCell = splitRec._cell;
		GridEdge edge = srcCell.getEdge(edgeNum);

		map<GridEdge, size_t> edgeToCellIdMap;
		for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
			const auto& vert = srcCell.getVert(_grid, p);
			const auto& cellIndices = vert.getCellIndices();
			for (size_t cellId : cellIndices) {
				const auto& adjCell = getCell(cellId);
				for (int i = 0; i < 12; i++) {
					GridEdge edge = adjCell.getEdge(i);
					if (srcCell.getVertsPos(edge.getVert(0)) != CVP_UNKNOWN && 
						srcCell.getVertsPos(edge.getVert(1)) != CVP_UNKNOWN)
							edgeToCellIdMap[edge] = cellId; // There may be several, use the last found
				}
			}
		}

		auto& verts = splitRec._verts;
		for (auto iter : edgeToCellIdMap) {
			const GridEdge& edge = iter.first;
			Vector3d midPt = edge.calcCenter(_grid);
			size_t cellId = iter.second;
			for (auto vertIter = verts.begin(); vertIter != verts.end(); vertIter++) {
				auto& vert = getVert(*vertIter);
				if (!needsClamp(*vertIter)) {
					verts.erase(vertIter--);
					continue;
				}
				const Vector3d& pt = vert.getPt();
				double dSqr = (pt - midPt).squaredNorm();
				if (dSqr < SAME_DIST_TOL_SQR) {
					vert.setClamp(_grid, TopolRef::createGridEdgeMidPtRef(edge));
					_clampedVerts.push_back(*vertIter);
					verts.erase(vertIter--);
				}
			}
		}
	}

	struct MatchRec {
		MatchRec(size_t vertIdx, const TopolRef& clamp)
			: _vertIdx(vertIdx),
			_clamp(clamp)
		{}

		size_t _vertIdx;
		TopolRef _clamp;
	};

	void CSplitter::clampToAdjacentCellFace(SplitSourceRec& splitRec, FaceNumber faceNum) {
		const auto& srcCell = splitRec._cell;
		size_t srcFaceIdx[4];
		srcCell.getFaceIndices(faceNum, srcFaceIdx);

		GridFace matchFace;
		for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
			const auto& vert = srcCell.getVert(_grid, p);
			const auto& cellIndices = vert.getCellIndices();
			for (size_t cellId : cellIndices) {
				const auto& adjCell = getCell(cellId);
				for (FaceNumber fn = BOTTOM; fn < FN_UNKNOWN; fn++) {
					size_t adjFaceIdx[4];
					adjCell.getFaceIndices(fn, adjFaceIdx);
					if (!GridFace::sameVerts(srcFaceIdx, adjFaceIdx))
						continue;
					matchFace = GridFace(cellId, fn);
					break;
				}
				if (matchFace.getCellIdx() != stm1)
					break;
			}
			if (matchFace.getCellIdx() != stm1)
				break;
		}

		if (matchFace.getCellIdx() == stm1) {
			return;
		}
		auto& verts = splitRec._verts;

		Vector3d faceMidPt = matchFace.calcCentroid(_grid);
		Vector3d faceNormal = matchFace.calcNormal(_grid);

		size_t faceIdx[4];
		matchFace.getVertIndices(_grid, faceIdx);

		map<double, vector<MatchRec>> matches;

		for (auto vertIter = verts.begin(); vertIter != verts.end(); vertIter++) {
			auto& vert = getVert(*vertIter);
			if (!needsClamp(*vertIter)) {
				verts.erase(vertIter--);
				continue;
			}

			const Vector3d& pt = vert.getPt();
			double dSqr = (pt - faceMidPt).squaredNorm();
			matches[dSqr].push_back(MatchRec(*vertIter, TopolRef::createGridFaceCentroidRef(matchFace)));

			size_t triIdx[2][2][3] = {
				{
					{ faceIdx[0], faceIdx[1], faceIdx[2] },
					{ faceIdx[0], faceIdx[1], faceIdx[2] },
				},
				{
					{ faceIdx[1], faceIdx[2], faceIdx[3] },
					{ faceIdx[1], faceIdx[3], faceIdx[0] },
				},
			};
			for (int orient = 0; orient < 2; orient++) {
				for (int triNum = 0; triNum < 2; triNum++) {
					const Vector3d* triPts[3]{
						&getVert(triIdx[orient][triNum][0]).getPt(),
						&getVert(triIdx[orient][triNum][1]).getPt(),
						&getVert(triIdx[orient][triNum][2]).getPt(),
					};
					Vector3d ctr = triangleCentroid(triPts);
					Vector3d normal = triangleNormal(triPts);
					Vector3d v = (pt - ctr);
					double distToPlane = fabs(v.dot(normal));
					if (distToPlane < SAME_DIST_TOL) {
						double dSqr = v.squaredNorm();
						matches[dSqr].push_back(MatchRec(*vertIter, TopolRef::createTriRef(triIdx[orient][triNum])));
					}
				}
			}
		}

		// By the time we get here, we really have no choice except to clamp
		int count = 0;
		while (!matches.empty() && matches.begin()->first < SAME_DIST_TOL_SQR) {
			const auto& matchArr = matches.begin()->second;
			for (const auto& match : matchArr) {
				auto& vert = getVert(match._vertIdx);
				vert.setClamp(_grid, match._clamp);
				_clampedVerts.push_back(match._vertIdx);

				auto iter = verts.find(match._vertIdx);
				if (iter != verts.end())
					verts.erase(iter);

				count++;
			}
			matches.erase(matches.begin());
		}

		if (!matches.empty()) {
			double dist = sqrt(matches.begin()->first);
			const auto& matchArr = matches.begin()->second;
			for (const auto& match : matchArr) {
				auto& vert = getVert(match._vertIdx);
				double maxMove = 0.25 * vert.findVertMinAdjEdgeLength(_grid);
				if (count == 2 || dist < maxMove) { // If count == 2, then we just clamped to exact triangles, the next in the list is the face center
					vert.setClamp(_grid, match._clamp);
					_clampedVerts.push_back(match._vertIdx);

					auto iter = verts.find(match._vertIdx);
					if (iter != verts.end())
						verts.erase(iter);

					break;
				}
			}
			matches.erase(matches.begin());
		}
	}

	void CSplitter::applyOctSplitClamps(SplitSourceRec& splitRec) {
		const auto& srcCell = splitRec._cell;
		size_t clampVertIdx;
		for (int i = 0; i < 12; i++) {
			GridEdge edge = srcCell.getEdge(i);
			Vector3d ctr = edge.calcCenter(_grid);
			if ((clampVertIdx = splitRec.findVert(ctr)) != stm1) {
				size_t meshIdx, polylineNum;
				if (clampedToSamePolyline(edge.getVert(0), edge.getVert(1), meshIdx, polylineNum))
					clampVertToPolyline(clampVertIdx, meshIdx, polylineNum);
				// remove vertex from list
			}
		}
	}

	void CSplitter::applyTriPrismSplitClamps(SplitSourceRec& splitRec) {
		const auto& srcCell = splitRec._cell;

		// We don't need orientation, just centers
		size_t vertIdx0 = srcCell.getVertIdx(splitRec._corner0);
		size_t vertIdx1 = srcCell.getVertIdx(splitRec._corner1);
		size_t meshIdx, polylineNum;
		if (clampedToSamePolyline(vertIdx0, vertIdx1, meshIdx, polylineNum)) {
			const auto& pt0 = getVert(vertIdx0).getPt();
			const auto& pt1 = getVert(vertIdx1).getPt();
			auto midPt = (pt0 + pt1) * 0.5;
			size_t vertIdx;
			if ((vertIdx = splitRec.findVert(midPt)) != stm1) {
				if (clampVertToPolyline(vertIdx, meshIdx, polylineNum)) {
					// remove vertex from list
				}
			} else
				cout << "Failed to find corner mid point\n";
		}

	}

	bool CSplitter::clampedToSamePolyline(size_t vert0, size_t vert, size_t& meshIdx, size_t& polylineNum) const {
		meshIdx = polylineNum = stm1;

		const auto& clamp0 = getVert(vert0).getClamp();
		const auto& clamp1 = getVert(vert).getClamp();

		if (!clamp0.matches(CLAMP_EDGE) && !clamp1.matches(CLAMP_EDGE))
			return false;

		if (!clamp0.matches(CLAMP_EDGE | CLAMP_VERT | CLAMP_FIXED) || !clamp1.matches(CLAMP_EDGE | CLAMP_VERT | CLAMP_FIXED))
			return false;

		if (clamp0.matches(CLAMP_FIXED | CLAMP_VERT)) {
			meshIdx = clamp1.getMeshIdx();
			polylineNum = clamp1.getPolylineNumber();
			return true;
		} else if (clamp1.matches(CLAMP_FIXED | CLAMP_VERT)) {
			meshIdx = clamp0.getMeshIdx();
			polylineNum = clamp0.getPolylineNumber();
			return true;
		} else {
			if (clamp0.getMeshIdx() != clamp1.getMeshIdx())
				return false;
			meshIdx = clamp0.getMeshIdx();

			if (clamp0.getPolylineNumber() == clamp1.getPolylineNumber()) {
				polylineNum = clamp0.getPolylineNumber();
				return true;
			}
		}
		return false;
	}

	void populateChiralityVerts(int chirality, const size_t verts[4], size_t tris[2][3]) {
		if (chirality == 0) {
			tris[0][0] = verts[0];
			tris[0][1] = verts[1];
			tris[0][2] = verts[2];

			tris[1][0] = verts[0];
			tris[1][1] = verts[2];
			tris[1][2] = verts[3];
		}
		else {
			tris[0][0] = verts[1];
			tris[0][1] = verts[2];
			tris[0][2] = verts[3];

			tris[1][0] = verts[1];
			tris[1][1] = verts[3];
			tris[1][2] = verts[0];
		}
	}

	bool CSplitter::clampVertToFaceTriVerts(size_t vertIdx) {
		if (isClamped(vertIdx))
			return false;

		auto& vert = getVert(vertIdx);
		set<size_t> adjCellIndices;
		vert.getAdjacentCellIndices(_grid, 0, adjCellIndices);

		const Vector3d& pt = vert.getPt();
		double minDist = DBL_MAX;
		bool tempCellIdxInList = false;
		for (size_t cellIdx : adjCellIndices) {
			const auto& cell = getCell(cellIdx);
			for (FaceNumber fn = BOTTOM; fn < FN_UNKNOWN; fn++) {
				size_t verts[4];
				cell.getFaceIndices(fn, verts);
				for (int chirality = 0; chirality < 2; chirality++) {
					size_t tris[2][3];
					populateChiralityVerts(chirality, verts, tris);

					for (int triNum = 0; triNum < 2; triNum++) {
						Vector3d ctr = _grid.calcTriangleCentroid(tris[triNum]);
						double dist = (pt - ctr).norm();
						if (dist < minDist)
							minDist = dist;
						if (dist < SAME_DIST_TOL) {
							vert.setClamp(_grid, TopolRef::createTriRef(tris[triNum]));
							_clampedVerts.push_back(vertIdx);
							return true;
						}
					}
				}
			}
		}
		return false;
	}

	bool CSplitter::getAdjacentVerts(size_t vertIdx, std::set<size_t>& edgeAdj, std::set<size_t>& cornerAdj) const {
		vector<GridFace> vertsFaces;
		if (_grid.getVertsFaces(vertIdx, false, vertsFaces) == 0)
			return false;

		for (const auto& face : vertsFaces) {
			const auto& cell = getCell(face.getCellIdx());
			size_t faceVerts[4];
			cell.getFaceIndices(face.getFaceNumber(), faceVerts);
			for (int i = 1; i <= 4; i++) {
				if (faceVerts[i % 4] != vertIdx) {
					if (faceVerts[(i - 1) % 4] == vertIdx || faceVerts[(i + 1) % 4] == vertIdx)
						edgeAdj.insert(faceVerts[i % 4]);
					else
						cornerAdj.insert(faceVerts[i % 4]);
				}
			}
		}

		return true;
	}

	bool CSplitter::clampVertToGridBoundary(size_t vertIdx) {
		if (isClamped(vertIdx))
			return false;

		auto& vert = getVert(vertIdx);
		set<size_t> adjVerts, cornerVerts;
		if (!getAdjacentVerts(vertIdx, adjVerts, cornerVerts))
			return false;

		vector<size_t> adj;
		for (size_t adjVertIdx : adjVerts) {
			const auto& adjClamp = getVert(adjVertIdx).getClamp();

			if (adjClamp.matches(CLAMP_PARALLEL | CLAMP_FIXED))
				adj.push_back(adjVertIdx);
		}

		if (adj.size() == 2) {
			const auto& vert0 = getVert(adj[0]);
			const auto& vert1 = getVert(adj[1]);
			const auto& pt0 = vert0.getPt();
			const auto& pt1 = vert1.getPt();
			auto v0 = (pt0 - vert.getPt()).normalized();
			auto v1 = (pt1 - vert.getPt()).normalized();
			if (fabs(v0.dot(v1)) > 0.7071) {
				vert.setClamp(_grid, TopolRef::createParallel(v0));
				_clampedVerts.push_back(vertIdx);
				return true;
			}
		}

		for (size_t vertIdx : adjVerts) {
			const auto& clamp = getVert(vertIdx).getClamp();

			if (clamp.getClampType() == CLAMP_PERPENDICULAR) {
				vert.setClamp(_grid, clamp);
				_clampedVerts.push_back(vertIdx);
				return true;
			}
		}

		for (size_t vertIdx : cornerVerts) {
			const auto& clamp = getVert(vertIdx).getClamp();

			if (clamp.getClampType() == CLAMP_PERPENDICULAR) {
				vert.setClamp(_grid, clamp);
				_clampedVerts.push_back(vertIdx);
				return true;
			}
		}

		return false;
	}

	bool CSplitter::clampVertToPolyline(size_t vertIdx, size_t meshIdx, size_t polylineNum) {
		if (isClamped(vertIdx))
			return false;

		auto& vert = getVert(vertIdx);
		const auto& pt = vert.getPt();

		double minDist = DBL_MAX;
		size_t bestPolylineNum;
		size_t bestPolylineIndex;
		Vector3d bestPt;

		const auto& mesher = _grid.getMesher();
		const auto& modelPtr = mesher.getModelPtr(meshIdx);
		const auto& pl = modelPtr->_polyLines[polylineNum];
		size_t plIdx;
		double d, t;
		pl.findClosestPointOnPolyline(*modelPtr, pt, plIdx, d, t);
		if (d < minDist) {
			minDist = d;
			bestPolylineNum = polylineNum;
			bestPolylineIndex = plIdx;
			LineSegment seg = pl.getSegment(*modelPtr, plIdx);
			if (t < 0)
				bestPt = seg._pts[0];
			else if (t > 1)
				bestPt = seg._pts[1];
			else
				bestPt = seg.interpolate(t);
		}
		

		if (minDist < DBL_MAX) {
			TopolRef newClamp = TopolRef::createPolylineRef(meshIdx, bestPolylineNum, bestPolylineIndex);
			setPoint(vertIdx, bestPt);
			vert.setClamp(_grid, newClamp);
			_clampedVerts.push_back(vertIdx);
			return true;
		}

		return false;
	}

	struct PolylineClampRec {
		vector<size_t> _vertClamps;
		map<size_t, vector<size_t>> _polylineClamps;
	};

	bool CSplitter::clampVertToCellEdgeMidPoint(size_t vertIdx) {
		if (isClamped(vertIdx))
			return false;

		auto& vert = getVert(vertIdx);
		const auto& pt = vert.getPt();

		set<size_t> adjCellIndices;
		vert.getAdjacentCellIndices(_grid, 0, adjCellIndices);
		for (size_t cellIdx : adjCellIndices) {
			const auto& cell = getCell(cellIdx);
			for (size_t edgeNumber = 0; edgeNumber < 12; edgeNumber++) {
				auto edge = cell.getEdge(edgeNumber);
				Vector3d midPt = edge.calcCenter(_grid);
				double d = (midPt - pt).squaredNorm();
				if (d < SAME_DIST_TOL_SQR) {
					TopolRef ref = TopolRef::createGridEdgeMidPtRef(edge);
					vert.setClamp(_grid, ref);
					_clampedVerts.push_back(vertIdx);
					return true;
				}
			}
		}

		return false;
	}

	inline size_t CSplitter::corn(CellVertPos p) const {
		return _workCell.getVertIdx(p);
	}

	inline size_t CSplitter::edge(CellVertPos p0, CellVertPos p1) {
		GridEdge ed(corn(p0), corn(p1));
		return _edgeCenters[ed];
	}

	inline size_t CSplitter::face(FaceNumber fn) const {
		return _faceCenters[fn];
	}

	inline const GridVert& CSplitter::getVert(size_t vertIdx) const {
		return _grid.getVert(vertIdx);
	}

	inline GridVert& CSplitter::getVert(size_t vertIdx) {
		return _grid.getVert(vertIdx);
	}

	inline const GridCell& CSplitter::getCell(size_t cellIdx) const {
		return _grid.getCell(cellIdx);
	}

	inline GridCell& CSplitter::getCell(size_t cellIdx) {
		return _grid.getCell(cellIdx);
	}

	bool CSplitter::isClamped(size_t vertIdx) const {
		const auto& vert = getVert(vertIdx);
		return vert.getClampType() != CLAMP_NONE;
	}

	bool CSplitter::needsClamp(size_t vertIdx) const {
		const auto& vert = getVert(vertIdx);
		double metric = vert.calcDegreesOfFreedomMetric(_grid);
		return metric > 1.0e-1;
	}

	CSplitter::SplitSourceRec::SplitSourceRec(SplitType type, CSplitter& splitter, size_t cellId)
		: _splitter(splitter)
		, _grid(splitter._grid)
		, _type(type)
		, _cell(splitter._grid.getCell(cellId))
	{
	}

	void CSplitter::SplitSourceRec::addChild(size_t cellId) {
		const auto& cell = _grid.getCell(cellId);
		for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
			size_t vertIdx = cell.getVertIdx(p);
			if (vertIdx < _splitter._numInitialVerts)
				continue;
			_verts.insert(vertIdx);
		}
	}

	size_t CSplitter::SplitSourceRec::findVert(const Vector3d& pt) const {
		for (const size_t vertIdx : _verts) {
			if (tolerantEquals(pt, _grid.getVert(vertIdx).getPt()))
				return vertIdx;
		}
		return stm1;
	}

	CSplitter::SplitSourceRec& CSplitter::createOctSplit(size_t cellId) {
		SplitSourceRec rec(SPLIT_OCT, *this, cellId);

		_cellsToClamp.push_back(rec);

		_grid.deleteCell(cellId);

		return _cellsToClamp.back();;
	}

	CSplitter::SplitSourceRec& CSplitter::createTriPrismSplit(size_t cellId,
		FaceNumber faceNumber, CellVertPos corner0, CellVertPos corner1) {
		SplitSourceRec rec(SPLIT_TRI_PRISM, *this, cellId);

		rec._faceNumber = faceNumber;
		rec._corner0 = corner0;
		rec._corner1 = corner1;
		_cellsToClamp.push_back(rec);

		_grid.deleteCell(cellId);

		return _cellsToClamp.back();;
	}

}
