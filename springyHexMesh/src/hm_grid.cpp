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

#include <hm_types.h>

#include <hm_tables.h>
#include <hm_grid.h>
#include <hm_gridEdge.h>
#include <hm_gridCell.h>
#include <hm_gridCellEnergy.h>
#include <hm_optimizer.h>
#include <meshProcessor.h>

namespace HexahedralMesher {

	Grid::Grid(CMesher& mesher)
		: _mesher(&mesher)
	{}

#define FOR_CV(AXIS, IDX) for(int IDX = 0; IDX < numDivs[AXIS]; IDX++)
#define FOR_CV2(AXIS, IDX) \
	minPt[AXIS] = bbox.getMin()[AXIS] + r[AXIS] * (IDX / ((double)numDivs[AXIS])); \
	maxPt[AXIS] = bbox.getMin()[AXIS] + r[AXIS] * ((IDX + 1) / ((double)numDivs[AXIS]))

	void Grid::init(const BoundingBox& bbox) {
		GridBase::setBounds(bbox);
		auto r = bbox.range();

		Vector3i numDivs;
		for (int i = 0; i < 3; i++) {
			numDivs[i] = (int)(r[i] / _mesher->getParams().calcMaxEdgeLength() + 0.5);
		}

		size_t numCells = numDivs[0] * numDivs[1] * numDivs[2];
		cout << "Number of intitial cells : " << numCells << "\n";
		Vector3d edgeLengths(r[0] / numDivs[0], r[1] / numDivs[1], r[2] / numDivs[2]);

// TODO		_cellStorage.reserve(numCells);
		size_t cellIdx = 0;
		Vector3d minPt, maxPt;
		FOR_CV(Z_AXIS, z) {
			FOR_CV2(Z_AXIS, z);
			FOR_CV(Y_AXIS, y) {
				FOR_CV2(Y_AXIS, y);
				FOR_CV(X_AXIS, x) {
					FOR_CV2(X_AXIS, x);
					GridCell cell;
					Vector3d r = maxPt - minPt;
					Vector3i idx;
					for (idx[Z_AXIS] = 0; idx[Z_AXIS] < 2; idx[Z_AXIS]++) {
						for (idx[Y_AXIS] = 0; idx[Y_AXIS] < 2; idx[Y_AXIS]++) {
							for (idx[X_AXIS] = 0; idx[X_AXIS] < 2; idx[X_AXIS]++) {
								Vector3d pt = minPt;

								pt[X_AXIS] += idx[X_AXIS] * r[X_AXIS];
								pt[Y_AXIS] += idx[Y_AXIS] * r[Y_AXIS];
								pt[Z_AXIS] += idx[Z_AXIS] * r[Z_AXIS];

								size_t vertIdx = add(pt);
								cell.setVertIdx(GridCell::vertPosOf(idx), vertIdx);
							}
						}
					}

					for (int i = 0; i < 12; i++) {
						GridEdge edge = cell.getEdge(i);
						Vector3d dir = edge.calcDir(*this);
						if (tolerantEquals(fabs(dir.dot(vX)), 1.0))
							cell.setRestEdgeLength(i, edgeLengths[X_AXIS]);
						else if (tolerantEquals(fabs(dir.dot(vY)), 1.0))
							cell.setRestEdgeLength(i, edgeLengths[Y_AXIS]);
						else if (tolerantEquals(fabs(dir.dot(vZ)), 1.0))
							cell.setRestEdgeLength(i, edgeLengths[Z_AXIS]);
						else
							throw "Bad cell edge during init";
					}

					addCell(cell);
				}
			}
		}

		size_t expectedVertCount = (numDivs[X_AXIS] + 1) * (numDivs[Y_AXIS] + 1) * (numDivs[Z_AXIS] + 1);
		if (expectedVertCount != numVerts()) {
			cout << "Unexpected vert count.\n";
		}

		if (!verify()) {
			cout << "Created an invald grid.\n";
		}

		bool verifyZeroEnergy = true, doEnergyTests = false;

		if (verifyZeroEnergy) {
			GridEnergy energyCal(*this);

			double totalCellEnergy = 0;
			iterateCells([&](size_t cellIdx)->bool {
				const auto& cell = getCell(cellIdx);
				totalCellEnergy += energyCal.calcTotalEnergy(cell);
				return true;
			});

			if (totalCellEnergy != 0) {
				cout << "TotalCellEnergy != 0 after init\n";
			}
		}
		int dbgBreak = 1;
	}

	bool Grid::verify() const {
		if (!GridBase::verify())
			return false;

		bool passed = true;
		iterateVerts([&](size_t vertIdx)->bool {
			const auto& vert = getVert(vertIdx);
			const auto& clamp = vert.getClamp();
			switch (clamp.getClampType()) {
			case CLAMP_EDGE: {
				auto modelPtr = _mesher->getModelPtr(clamp.getMeshIdx());
				if (!modelPtr)
					return false;

				const auto& pl = modelPtr->_polyLines[clamp.getPolylineNumber()];
				size_t plIdx;
				double d, t;
				if (!pl.findClosestPointOnPolyline(*modelPtr, vert.getPt(), plIdx, d, t))
					return false;
				if (d > SAME_DIST_TOL)
					return false;
				if (plIdx != clamp.getPolylineIndex())
					return false;

				break;
			}
			default:
				break;
			}
			return true;
		}, 1);

		if (!passed)
			return false;

		return true;
	}

	const ParamsRec& Grid::getParams() const {
		return _mesher->getParams();
	}

	size_t Grid::getVertsFaces(size_t vertIdx, bool includeOpposedPairs, std::vector<GridFace>& faceRefs) const {
		faceRefs.clear();

		const vector<size_t>& cellIndices = getVert(vertIdx).getCellIndices();
		for (size_t cellIdx : cellIndices) {
			const auto& cell = getCell(cellIdx);
			CellVertPos p = cell.getVertsPos(vertIdx);
			for (int i = 0; i < 3; i++) {
				faceRefs.push_back(GridFace(cellIdx, gPosFaceNumberLUT[p][i]));
			}
		}
		if (!includeOpposedPairs) {
			for (size_t i = 0; i < faceRefs.size(); i++) {
				for (size_t j = i + 1; j < faceRefs.size(); j++) {
					Vector3d normalI = faceRefs[i].calcNormal(*this);
					Vector3d normalJ = faceRefs[j].calcNormal(*this);
					if (normalI.dot(normalJ) < -0.7071) {
						faceRefs.erase(faceRefs.begin() + j);
						faceRefs.erase(faceRefs.begin() + i);
						i--; // move back one to get the next entry
						break;
					}
				}
			}
		}
		return faceRefs.size();
	}

	double Grid::minimizeVertexEnergy(std::ostream& logOut, size_t vertIdx, int clampMask) {
		GridVert& vert = getVert(vertIdx);

		// This vertex can't move
		Vector3d gradient;
		const TopolRef& clamp = vert.getClamp();
		int minMask = CLAMP_NONE | CLAMP_EDGE | CLAMP_PERPENDICULAR | CLAMP_PARALLEL | CLAMP_GRID_TRI_PLANE;
		if (!clamp.matches(clampMask & minMask))
			return 0;

		int logMask = CLAMP_EDGE;
		if (clamp.matches(logMask))
			logOut << "Vert: " << vertIdx << " type: " << clamp.getClampType() << "\n";
		auto logFunc = [&](int count, double moveDist) {
			if (clamp.matches(logMask))
				logOut << "  " << count << ", moveDist: " << moveDist << "\n";
		};

		switch (clamp.getClampType()) {
		case CLAMP_NONE:
			return minimizeVertexEnergy(vertIdx, logFunc,
				[&](double dt, Vector3d& gradient)->double {
					return calcEnergyGradientFree(vertIdx, dt, gradient);
				});
		case CLAMP_EDGE: {
			return minimizeVertexEnergy(vertIdx, logFunc, [&](double dt, Vector3d& gradient)->double {
				return calcEnergyGradientEdge(vertIdx, dt, gradient);
			});
		}
		case CLAMP_PERPENDICULAR:
			return minimizeVertexEnergy(vertIdx, logFunc, [&](double dt, Vector3d& gradient)->double {
				return calcEnergyGradientPerpendicular(vertIdx, clamp.getVector(), dt, gradient);
			});
		case CLAMP_PARALLEL:
			return minimizeVertexEnergy(vertIdx, logFunc, [&](double dt, Vector3d& gradient)->double {
				gradient = clamp.getVector();
				return DBL_MAX;
			});
		case CLAMP_GRID_TRI_PLANE:
			return minimizeVertexEnergy(vertIdx, logFunc, [&](double dt, Vector3d& gradient)->double {
				return calcEnergyGradientTriPlane(vertIdx, dt, gradient);
			});
		default:
			break;
		}

		return 0;
	}

	namespace {
		const double maxDistFactor = 0.125;
	}

	double Grid::calcEnergyGradientFree(size_t vertIdx, double dt, Vector3d& gradient) {
		gradient = Vector3d(0, 0, 0);

		GridVert& vert = getVert(vertIdx);
		Vector3d originalPos = vert.getPt();
		Vector3d displacedPos;
		double e0, e1;
		e0 = calcVertexEnergy(vertIdx);
		if (e0 <= 1.0e-6)
			return 0;

		for (Axis axis = X_AXIS; axis <= Z_AXIS; axis++) {
			displacedPos = originalPos;

			displacedPos[axis] += dt;
			e1 = calcVertexEnergyAtPos(vertIdx, displacedPos);

			double slope = (e1 - e0) / dt;
			gradient[axis] = slope;
		}

		double mag = gradient.norm();
		if (mag > minNormalizeDivisor)
			gradient = gradient / mag;

		fixGradientDirection(vertIdx, dt, gradient);

		return DBL_MAX;
	}

	double Grid::calcEnergyGradientPerpendicular(size_t vertIdx, const Vector3d& normal, double dt, Vector3d& gradient) {
		gradient = Vector3d(0, 0, 0);

		Vector3d xAxis = vX;
		if (fabs(xAxis.dot(normal)) > 0.7071) {
			xAxis = vY;
			if (fabs(xAxis.dot(normal)) > 0.7071) {
				xAxis = vZ;
			}
		}
		xAxis = (xAxis - normal * normal.dot(xAxis)).normalized();
		Vector3d yAxis = normal.cross(xAxis);

		double e0, e1;
		auto& vert = getVert(vertIdx);
		Vector3d originalPos = vert.getPt();
		e0 = calcVertexEnergy(vertIdx);
		if (e0 < 1.0e-6)
			return 0;

		for (int i = 0; i < 2; i++) {
			Vector3d dir((i == 0 ? xAxis : yAxis));

			e1 = calcVertexEnergyAtPos(vertIdx, originalPos + dt * dir);

			double slope = (e1 - e0) / dt;
			gradient += slope * dir;
		}

		double mag = gradient.norm();
		if (mag > minNormalizeDivisor)
			gradient = gradient / mag;

		fixGradientDirection(vertIdx, dt, gradient);

		return DBL_MAX;
	}

	double Grid::calcMoveDist(size_t vertIdx, double dt, const Vector3d& gradient) {
		auto calFunc = [&](const Vector3d& pt)->double {
			return calcVertexEnergyAtPos(vertIdx, pt);
		};

		auto& pt0 = getVert(vertIdx).getPt();
		return SteepestAcent<Vector3d>::calcMoveDist(pt0, dt, gradient, calFunc);
	}

	void Grid::fixGradientDirection(size_t vertIdx, double dt, Vector3d& gradient) {
		double moveDist = calcMoveDist(vertIdx, dt, gradient);
		if (moveDist < 0)
			gradient = -gradient;
	}

	double Grid::calcEnergyGradientEdge(size_t vertIdx, double dt, Vector3d& gradient) {
		gradient = Vector3d(0, 0, 0);
		int numGradients = 1;
		Vector3d gradients[2];
		double len[2];

		auto safeNormalize = [&]() {
			for (int i = 0; i < numGradients; i++) {
				len[i] = gradients[i].norm();
				if (len[i] < minNormalizeDivisor)
					len[i] = 0;
				else
					gradients[i] /= len[i];
			}
		};

		GridVert& vert = getVert(vertIdx);
		const TopolRef& clamp = vert.getClamp();
		if (clamp.getClampType() != CLAMP_EDGE) {
			throw "Bad clamp";
		}
		const CModelPtr& modelPtr = _mesher->getModelPtr(clamp.getMeshIdx());
		const TriMesh::CPolyLine& pl = modelPtr->_polyLines[clamp.getPolylineNumber()];
		size_t plIdx;
		double d, t;
		bool needsClamp = !pl.findClosestPointOnPolyline(*modelPtr, vert.getPt(), plIdx, d, t);

		// Point is not precisely on the line
		LineSegment seg = pl.getSegment(*modelPtr, plIdx);
		if (plIdx != clamp.getPolylineIndex()) {
//			cout << "Vert: " << vertIdx << " shifted plIdx from " << clamp.getPolylineIndex() << " to " << plIdx << "\n";
			vert.getClamp().setPolylineIndex(plIdx);
		}

		double segLen = seg.calLength();
		Vector3d clampPt;
		if (t < 0) {
			// Can only reach this block if the closest point is off the polyline and before it starts
			// For all other segments, the prior segment test will detect falling on the vertex before
			// we reach this point.
			cout << "Optimum location is before the start of the edge.\n";
			vert.setPoint(seg._pts[0]);
			needsClamp = false;

			if (plIdx == 0) {
				gradients[0] = seg._pts[1] - seg._pts[0];
				safeNormalize();
			}
		} else if (t * segLen > segLen - SAME_DIST_TOL) {
			if (d > SAME_DIST_TOL)
				cout << "Current pt not on vertex\n";
			vert.setPoint(seg._pts[1]);
			needsClamp = false;

			if (plIdx == pl.getVerts().size() - 2) {
				gradients[0] = seg._pts[0] - seg._pts[1];
				safeNormalize();
			}
			else {
				gradients[0] = seg._pts[0] - seg._pts[1];
				seg = pl.getSegment(*modelPtr, plIdx + 1);
				gradients[1] = seg._pts[1] - seg._pts[0];
				numGradients = 2;
				safeNormalize();
			}
		} else {
			clampPt = seg.interpolate(t);
			gradients[0] = seg._pts[0] - clampPt;
			gradients[1] = seg._pts[1] - clampPt;
			numGradients = 2;
			safeNormalize();
		}

		if (needsClamp) {
			double dist = (clampPt - vert.getPt()).norm();
			cout << "Vert " << vertIdx << " not on line by d = " << d << "\n";
			vert.setPoint(clampPt); // Lock it on the line
		}

		int idx = -1;
		if (numGradients > 1)
			idx = chooseBestGradient(vertIdx, dt, gradients, numGradients);

		if (idx == -1)
			return 0;
		gradient = gradients[idx];
		return len[idx];
	}

	int Grid::chooseBestGradient(size_t vertIdx, double dt, const Vector3d gradients[], int numGradients) {
		int result = -1;
		double maxSlope = 0;
		const auto& curPos = getVert(vertIdx).getPt();
		for (int i = 0; i < numGradients; i++) {
			double tMin = calcMoveDist(vertIdx, dt, gradients[i]);
			if (tMin > maxSlope) {
				maxSlope = tMin;
				result = i;
			}
		}
		return result;
	}

	double Grid::calcEnergyGradientTriPlane(size_t vertIdx, double dt, Vector3d& gradient) {
		const auto& vert = getVert(vertIdx);
		const auto& clamp = vert.getClamp();
		size_t triIdx[3];
		clamp.getTriVertIndices(triIdx);
		const Vector3d* triPts[3] = {
			&getVert(triIdx[0]).getPt(),
			&getVert(triIdx[1]).getPt(),
			&getVert(triIdx[2]).getPt(),
		};

		Vector3d normal = triangleNormal(triPts);
		return calcEnergyGradientPerpendicular(vertIdx, normal, dt, gradient);
	}

	double Grid::clampVertex(size_t vertIdx) {
		const auto& vert = getVert(vertIdx);

		auto p0 = vert.getPt();
		switch (vert.getClampType()) {
		case CLAMP_CELL_EDGE_CENTER:
			return clampVertexToCellEdgeCenter(vertIdx);
		case CLAMP_CELL_FACE_CENTER:
			clampVertexToCellFaceCenter(vertIdx);
			return 0;
		case CLAMP_GRID_TRI_PLANE:
			clampVertexToTriPlane(vertIdx);
			return 0;
		default:
			return 0;
		}
	}

	double Grid::clampVertexToCellFaceCenter(size_t vertIdx) {
		auto& vert = getVert(vertIdx);
		const auto& clamp = vert.getClamp();
		if (clamp.getClampType() != CLAMP_CELL_FACE_CENTER)
			return 0;

		const auto& cell = getCell(clamp.getCellIdx());
		if (cellExists(clamp.getCellIdx())) {
			FaceNumber face = clamp.getFaceNumber();
			Vector3d newPt = cell.calcFaceCentroid(*this, face);
			double dist = (newPt - vert.getPt()).norm();
			if (dist > OPTIMIZER_TOL)
				vert.setPoint(newPt);
			return dist;
		}
		else {
			cout << "Linkage error: Vert(" << vertIdx << ") is clamped to the edge of a cell that does not exist.\n";
		}
		return 0;
	}

	double Grid::clampVertexToTriPlane(size_t vertIdx) {
		auto& vert = getVert(vertIdx);
		const auto& clamp = vert.getClamp();
		if (clamp.getClampType() != CLAMP_GRID_TRI_PLANE)
			return 0;

		size_t triVertIndices[3];
		clamp.getTriVertIndices(triVertIndices);
		const Vector3d* triPts[3] = {
			&getVert(triVertIndices[0]).getPt(),
			&getVert(triVertIndices[1]).getPt(),
			&getVert(triVertIndices[2]).getPt(),
		};
		Vector3d newPt = vert.getPt();
		Plane pl(*triPts[0], triangleNormal(triPts));
		double dist = distanceFromPlane(newPt, pl);

		if (dist > OPTIMIZER_TOL) {
			newPt -= dist * pl._normal;
			vert.setPoint(newPt);
		}
		return dist;
	}

	double Grid::clampVertexToCellEdgeCenter(size_t vertIdx) {
		auto& vert = getVert(vertIdx);
		const auto& clamp = vert.getClamp();
		if (clamp.getClampType() != CLAMP_CELL_EDGE_CENTER)
			return DBL_MAX;

		GridEdge edge = clamp.getEdge();
		auto newPt = edge.calcCenter(*this);
		auto curPt = vert.getPt();
		double dist = (newPt - curPt).norm();
		if (dist > OPTIMIZER_TOL)
			vert.setPoint(newPt);
		return dist;
	}	

	template<typename GRAD_FUNC, typename LOG_FUNC>
	double Grid::minimizeVertexEnergy(size_t vertIdx, LOG_FUNC logFunc, GRAD_FUNC calGrad) {
		ScopedSetStash restore(*this, vertIdx);

		auto& vert = getVert(vertIdx);
		const int maxOptimizerSteps = 10;
		const double maxMove = 0.25 * vert.findVertMinAdjEdgeLength(*this);
		const double differentialDist = 1.0e-8;
		const double minEnergy = 1.0e-4;

		double dist = 0;
		Vector3d originalPos = vert.getPt();

		auto calFunc = [&](const Vector3d& pt)->double {
			return calcVertexEnergyAtPos(vertIdx, pt);
		};

		SteepestAcent<Vector3d> asc(minEnergy, differentialDist);
		dist = asc.run(vert.getPt(), maxOptimizerSteps, maxMove, calFunc, calGrad, logFunc);

		return dist;
	}

	double Grid::calcCellEnergy(size_t cellId) const {
		GridEnergy eCal(*this);
		const GridCell& cell = getCell(cellId);
		return eCal.calcTotalEnergy(cell);
	}

	double Grid::calcVertexEnergy(size_t vertIdx) const {
		GridEnergy eCal(*this);
		const GridVert& vert = getVert(vertIdx);
		return eCal.calcTotalEnergy(vert);
	}

	double Grid::calcVertexEnergyAtPos(size_t vertIdx, const Vector3d& atPt) {
		GridEnergy eCal(*this);
		GridVert& vert = getVert(vertIdx);
		Vector3d originalPt = vert.getPt();

		vert.setPoint(atPt);
		double result = eCal.calcTotalEnergy(vert);
		vert.setPoint(originalPt);

		return result;
	}

	double Grid::calcVertexOrthoEnergy(size_t vertIdx) const {
		GridEnergy eCal(*this);
		const GridVert& vert = getVert(vertIdx);
		return eCal.calcBendEnergy(vert);
	}

	double Grid::calcVertexOrthoEnergyAtPos(size_t vertIdx, const Vector3d& atPt) {
		GridEnergy eCal(*this);
		GridVert& vert = getVert(vertIdx);
		Vector3d originalPt = vert.getPt();

		vert.setPoint(atPt);
		double result = eCal.calcBendEnergy(vert);
		vert.setPoint(originalPt);

		return result;
	}

	Vector3d Grid::calcTriangleCentroid(size_t vertIdx[3]) const {
		const Vector3d* pts[3] = {
			&getVert(vertIdx[0]).getPt(),
			&getVert(vertIdx[1]).getPt(),
			&getVert(vertIdx[2]).getPt(),
		};
		return triangleCentroid(pts);
	}

	Grid::ScopedSetStash::ScopedSetStash(Grid& grid, size_t vertIdx)
		: _grid(grid)
		, _vertIdx(vertIdx)
	{
		const auto& vert = _grid.getVert(vertIdx);

		_pt = vert.getPt();
		_clamp = vert.getClamp();
	}

	Grid::ScopedSetStash::~ScopedSetStash() {
		auto& vert = _grid.getVert(_vertIdx);

		vert.setStashPoint(vert.getPt());
		vert.setStashClamp(_grid, vert.getClamp());

		vert.setPoint(_pt);
		vert.setClamp(_grid, _clamp);
	}

}