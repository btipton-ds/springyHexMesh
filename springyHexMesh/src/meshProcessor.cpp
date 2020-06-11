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
#include <fstream>
#include <set>

#include <hm_types.h>
#include <meshProcessor.h>
#include <hm_polylineFitter.h>
#include <hm_splitter.h>
#include <hm_grid.h>
#include <readSTL.h>

using namespace HexahedralMesher;

using namespace std;
const double edgeFrac = 0.5;
constexpr size_t CTR_IDX = stm1 - 1;

#ifdef __linux__
const std::string directorRoot("/mnt/c/Users/Bob/Documents/Projects/ElectroFish/");
#else
const std::string directorRoot("/Users/Bob/Documents/Projects/ElectroFish/");
#endif // __linux__
const std::string dumpPath(directorRoot + "HexMeshTests/dumps/");
const std::string savePath(directorRoot + "HexMeshTests/");

#define DUMP_OBJ 0

namespace {
	class StopException {
	};
}

Vector3d quadCenter(Vector3d const* const quadPts[4]) {
	Vector3d ctr(0, 0, 0);
	for (int i = 0; i < 4; i++)
		ctr += *quadPts[i];
	ctr /= 4;
	return ctr;
}

CMesher::CMesher(const ParamsRec& params)
	: _params(params)
	, _grid(make_shared<Grid>(*this))
	, _dumpObj(*_grid, dumpPath)
{
}

CMesher::~CMesher() {
	if (_thread) {
		_thread->join();
		delete _thread;
	}
}

void CMesher::checkStop() const {
	if (_reporter && !_reporter->isRunning()) {
		throw StopException();
	}
}

void CMesher::reset() {
	_modelPtrs.clear();
}

bool CMesher::addFile(const std::string& path, const std::string& filename) {
	CModelPtr modelPtr = make_shared<CModel>(_params.sharpAngleDeg);
	CReadSTL reader(modelPtr);
	if (!reader.read(path, filename))
		return false;

	modelPtr->init();
	addModel(static_pointer_cast<CModel> (reader.getMeshPtr()));

	return true;
}

void CMesher::addModel(const std::shared_ptr<CModel>& modelPtr) {
	if (_reporter)
		_reporter->reportModelAdded(*this, modelPtr);
	_modelPtrs.push_back(modelPtr);
}

void CMesher::save(const string& path) const {
	ofstream out(path);
	save(out);
}

void CMesher::save(ostream& out) const {
	if (!_grid->verify()) {
		cout << "Failed to save because grid could not be verifid.\n";
		return;
	}

	out << "Mesher version 1\n";
		
	_params.save(out);

	_grid->save(out);

	out << "NM: " << _modelPtrs.size() << "\n";
	for (size_t i = 0; i < _modelPtrs.size(); i++) {
		_modelPtrs[i]->save(out);
	}

}

bool CMesher::read(const std::string& path) {
	ifstream in(path);
	if (!in.good())
		return false;
	return read(in);
}

bool CMesher::read(std::istream& in) {
	string str0, str1;
	int version;

	in  >> str0 >> str1 >> version;
	if (str0 != "Mesher" || str1 != "version")
		return false;

	ParamsRec newParams(_params);
	if (!newParams.read(in))
		return false;


	GridPtr newGrid = make_shared<Grid>(*this);
	if (!newGrid->read(in))
		return false;

	size_t numModels;
	in >> str0 >> numModels;
	if (str0 != "NM:")
		return false;

	vector<CModelPtr> newModels;
	for (size_t i = 0; i < numModels; i++) {
		CModelPtr model = make_shared<CModel>(_params.sharpAngleDeg);
		if (!model->read(in))
			return false;
		newModels.push_back(model);
	}

	_params = newParams;
	_grid = newGrid;
	_modelPtrs = newModels;
	return true;
}

void CMesher::splitCellsAroundPolylines() {
		map<size_t, set<size_t>> cellPolylineHits;
		_grid->iterateCells([&](size_t cellId)->bool {
			const auto& cell = _grid->getCell(cellId);
			auto bb = cell.calcBBox(*_grid);
			for (const auto& modelPtr : _modelPtrs) {
				for (size_t polylineNum = 0; polylineNum < modelPtr->_polyLines.size(); polylineNum++) {
					const auto& verts = modelPtr->_polyLines[polylineNum].getVerts();
					for (size_t vertIdx : verts) {
						const auto& modelPt = modelPtr->getVert(vertIdx)._pt;
						if (bb.contains(modelPt)) {
							cellPolylineHits[cellId].insert(polylineNum);
							break;
						}
					}
				}
			}
			return true;
		});

		vector<size_t> cellsToSplit;
		size_t maxInCell = 0;
		for (const auto& iter : cellPolylineHits) {
			cellsToSplit.push_back(iter.first);
			size_t n = iter.second.size();
			if (n > maxInCell)
				maxInCell = n;
		}

		if (maxInCell > 1) {
			for (int i = 0; i < 2; i++) {
				CSplitter splitter(*_grid);
				for (size_t cellId : cellsToSplit) {
					splitter.splitCellFull(cellId);
				}
				cellsToSplit = splitter.getNewCells();
			}
		}
		
}

void CMesher::snapToCusps() {
	for (size_t modelIdx = 0; modelIdx < _modelPtrs.size(); modelIdx++) {
		const auto& modelPtr = _modelPtrs[modelIdx];
		const auto& cusps = modelPtr->_cusps;
		for (size_t cuspVertIdx : cusps) {
			const auto& cuspPt = modelPtr->getVert(cuspVertIdx)._pt;
			if (!_params.bounds.contains(cuspPt))
				continue;
			BoundingBox bb(cuspPt);
			double maxEdgeLength = _params.calcMaxEdgeLength();
			bb.grow(1.5 * maxEdgeLength);
			vector<size_t> vertIndices = _grid->findVerts(bb);
			double minDist = 1.5 * maxEdgeLength;
			size_t bestSnap = stm1;
			Vector3d snapPt;
			for (size_t gridVertIdx : vertIndices) {
				const auto& gridPt = _grid->getVert(gridVertIdx).getPt();
				auto v = gridPt - cuspPt;
				double dist = fabs(v[0]) + fabs(v[1]) + fabs(v[2]);
				if (dist < minDist) {
					minDist = dist;
					bestSnap = gridVertIdx;
					snapPt = cuspPt;
				}
			}
			if (bestSnap != stm1) {
				auto& vert = _grid->getVert(bestSnap);
				vert.setPoint(snapPt);
				vert.setClamp(*_grid, TopolRef::createVert(modelIdx, bestSnap));
			} else
				cout << "Failed to snap cusp.\n";
		}
	}
}

double CMesher::findMinimumGap() const {
	double minGap = FLT_MAX;

	for (const auto& meshPtr : _modelPtrs) {
		double d = meshPtr->findMinGap();
		if (d < minGap)
			minGap = d;
	}

#if 0 // Get histogram
	double range = 0;
	for (const auto& meshPtr : _modelPtrs) {
		double r = meshPtr->getBBox().range().norm();
		if (r > range) {
			range = r;
		}
	}
	vector<double> binSizes;
	vector<size_t> bins;
	double r = range / 1000;
	while (r < range) {
		binSizes.push_back(r);
		r *= 1.5;
	}
	bins.resize(binSizes.size());

	double numTris = 0;
	for (const auto& meshPtr : _modelPtrs) {
		numTris += meshPtr->numTris();
		meshPtr->getGapHistogram(binSizes, bins);
	}

	cout << "Gab histogram\n";
	for (size_t i = 0; i < binSizes.size(); i++)
		cout << binSizes[i] << ": " << (100 * bins[i] / numTris) << "%\n";
#endif

	return minGap;
}

size_t CMesher::findVertFaces(size_t vertIdx, set<GridFace>& faceSet) const {
	faceSet.clear();
	const auto& vert = _grid->getVert(vertIdx);
	size_t numCells = vert.getNumCells();
	for (size_t i = 0; i < numCells; i++) {
		size_t cellIdx = vert.getCellIndex(i);
		const auto& cell = _grid->getCell(cellIdx);
		GridFace vertFaceSet[3];
		CellVertPos pos = cell.getVertsPos(vertIdx);
		cell.getVertGridFaces(cellIdx, pos, vertFaceSet);
		for (int i = 0; i < 3; i++)
			faceSet.insert(vertFaceSet[i]);
	}

	return faceSet.size();
}

void CMesher::clampBoundaryPlane(size_t vertIdx) {
	auto& vert = _grid->getVert(vertIdx);

	const vector<size_t>& cellIndices = vert.getCellIndices();
	if (cellIndices.size() != 4) {
		throw "Function only works for the 4 cell case.";
	}

	vector<Vector3d> norms;
	for (size_t cellIdx : cellIndices) {
		const GridCell& cell = _grid->getCell(cellIdx);
		size_t vertIndices[3];
		cell.getVertsEdgeIndices(cell.getVertsPos(vertIdx), vertIndices);
		for (int i = 0; i < 3; i++) {
			const auto& pt = _grid->getVert(vertIndices[i]).getPt();
			norms.push_back((pt - vert.getPt()).normalized());
		}
	}

	int hits[] = { 0, 0, 0, 0, 0, 0 };
	for (size_t i = 0; i < norms.size(); i++) {
		if (norms[i].dot(vX) > 0.7071)
			hits[X_POS]++;
		else if (norms[i].dot(vX) < -0.7071)
			hits[X_NEG]++;
		else if (norms[i].dot(vY) > 0.7071)
			hits[Y_POS]++;
		else if (norms[i].dot(vY) < -0.7071)
			hits[Y_NEG]++;
		else if (norms[i].dot(vZ) > 0.7071)
			hits[Z_POS]++;
		else if (norms[i].dot(vZ) < -0.7071)
			hits[Z_NEG]++;
	}

	if (hits[X_POS] == 0 || hits[X_NEG] == 0)
		vert.setClamp(*_grid, TopolRef::createPerpendicular(vX));
	else if (hits[Y_POS] == 0 || hits[Y_NEG] == 0)
		vert.setClamp(*_grid, TopolRef::createPerpendicular(vY));
	else if (hits[Z_POS] == 0 || hits[Z_NEG] == 0)
		vert.setClamp(*_grid, TopolRef::createPerpendicular(vZ));
	else
		throw "Normal not aligned with a principal axis.";

}

void CMesher::clampBoundaryEdge(size_t vertIdx) {
	auto& vert = _grid->getVert(vertIdx);

	const vector<size_t>& cellIndices = vert.getCellIndices();
	if (cellIndices.size() != 2) {
		throw "Function only works for the 2 cell case.";
	}

	vector<Vector3d> norms;
	for (size_t cellIdx : cellIndices) {
		const GridCell& cell = _grid->getCell(cellIdx);
		size_t vertIndices[3];
		cell.getVertsEdgeIndices(cell.getVertsPos(vertIdx), vertIndices);
		for (int i = 0; i < 3; i++) {
			const auto& pt = _grid->getVert(vertIndices[i]).getPt();
			norms.push_back((pt - vert.getPt()).normalized());
		}
	}

	int hits[] = { 0, 0, 0 };
	for (size_t i = 0; i < norms.size(); i++) {
		if (norms[i].dot(vX) < -0.7071)
			hits[X_AXIS]++;
		else if (norms[i].dot(vY) < -0.7071)
			hits[Y_AXIS]++;
		else if (norms[i].dot(vZ) < -0.7071)
			hits[Z_AXIS]++;
	}

	if (hits[X_AXIS] == 1)
		vert.setClamp(*_grid, TopolRef::createParallel(vX));
	else if (hits[Y_AXIS] == 1)
		vert.setClamp(*_grid, TopolRef::createParallel(vY));
	else if (hits[Z_AXIS] == 1)
		vert.setClamp(*_grid, TopolRef::createParallel(vZ));
	else
		throw "Normal not aligned with a principal axis.";
}

void CMesher::clampBoundaryCorner(size_t vertIdx) {
	auto& vert = _grid->getVert(vertIdx);
	vert.setClamp(*_grid, TopolRef::createFixed());
}

void CMesher::clampBoundaries() {
	_grid->iterateVerts([&](size_t vertIdx)->bool {
		auto& v = _grid->getVert(vertIdx);
		if (v.getClampType() != CLAMP_NONE)
			return true;
		size_t numCells = v.getNumCells();
		switch (numCells) {
		case 8:
			// Fall through and do nothing
			break;
		case 4:
			clampBoundaryPlane(vertIdx);
			break;
		case 2:
			clampBoundaryEdge(vertIdx);
			break;
		case 1:
			clampBoundaryCorner(vertIdx);
			break;
		default:
			throw ("Should never happen");
		}

		return true;
		});
}

void CMesher::intersectMesh() {
}

void CMesher::minimizeMesh(int steps, int energyMask, const string& filename) {
	_grid->clearSearchTrees();

	if (steps < 0) {
#ifdef _DEBUG
		steps = 25;
#else
		steps = 5000;
#endif
	}

	ofstream logOut(savePath + "opt_log.csv");

	const int numThreads = 4;
	for (int i = 0; i < steps; i++) {
		checkStop();
		double maxMoveArr[numThreads], avgMoveArr[numThreads];
		double maxEnergyArr[numThreads], avgEnergyArr[numThreads];
		for (int j = 0; j < numThreads; j++) {
			maxMoveArr[j]= avgMoveArr[j] = maxEnergyArr[j] = avgEnergyArr[j] = 0;
		}

		_grid->iterateVerts([&](size_t vertIdx)->bool {
			auto& vert = _grid->getVert(vertIdx);
			vert.copyToThread();
			return true;
		}, numThreads);

		_grid->iterateVerts([&](size_t vertIdx)->bool {
			size_t threadNum = Grid::getThreadNumber();

			if (vertIdx == 164) {
				int dbgBreak = 1;
			}

			double move = _grid->minimizeVertexEnergy(logOut, vertIdx, energyMask);

			if (move > maxMoveArr[threadNum])
				maxMoveArr[threadNum] = move;
			double e = _grid->calcVertexEnergy(vertIdx);

			if (vertIdx == 164) {
				_grid->calcVertexEnergy(vertIdx);
			}

			if (e > maxEnergyArr[threadNum])
				maxEnergyArr[threadNum] = e;

			avgMoveArr[threadNum] += move;
			avgEnergyArr[threadNum] += e;
			return true;
			}, numThreads);

		_grid->iterateVerts([&](size_t vertIdx)->bool {
			auto& vert = _grid->getVert(vertIdx);
			vert.copyFromThread();
			return true;
			}, numThreads);

		double avgMoveClamp;

		avgMoveClamp = DBL_MAX;
		for (int i = 0; i < 3 && avgMoveClamp > 1.0e-5; i++) {
			double avgMoveClampArr[numThreads];
			size_t numClamps[numThreads];
			for (int i = 0; i < numThreads; i++) {
				avgMoveClampArr[i] = 0;
				numClamps[i] = 0;
			}

			_grid->iterateVerts([&](size_t vertIdx)->bool {
				size_t threadNum = Grid::getThreadNumber();
				double dist = _grid->clampVertexToCellEdgeCenter(vertIdx);
				if (dist < DBL_MAX) {
					avgMoveClampArr[threadNum] += fabs(dist);
					numClamps[threadNum]++;
				}
				return true;
			}, numThreads);

			_grid->iterateVerts([&](size_t vertIdx)->bool {
				size_t threadNum = Grid::getThreadNumber();
				double dist = _grid->clampVertexToTriPlane(vertIdx);
				if (dist < DBL_MAX) {
					avgMoveClampArr[threadNum] += fabs(dist);
					numClamps[threadNum]++;
				}
				return true;
			}, numThreads);

			_grid->iterateVerts([&](size_t vertIdx)->bool {
				size_t threadNum = Grid::getThreadNumber();
				double dist = _grid->clampVertexToCellFaceCenter(vertIdx);
				if (dist < DBL_MAX) {
					avgMoveClampArr[threadNum] += fabs(dist);
					numClamps[threadNum]++;
				}
				return true;
			}, numThreads);
			_grid->iterateVerts([&](size_t vertIdx)->bool {
				auto& vert = _grid->getVert(vertIdx);
				vert.copyFromThread();
				return true;
			}, numThreads);

			avgMoveClamp = 0;
			for (int i = 0; i < numThreads; i++) {
				if (numClamps[i] > 0)
					avgMoveClamp += avgMoveClampArr[i] / (double)numClamps[i];
			}

			avgMoveClamp /= (double)numThreads;
		}

		double maxMove = 0, avgMove = 0;
		double maxEnergy = 0, avgEnergy = 0;
		for (int j = 0; j < numThreads; j++) {
			if (maxMoveArr[j] > maxMove)
				maxMove = maxMoveArr[j];
			if (maxEnergyArr[j] > maxEnergy)
				maxEnergy = maxEnergyArr[j];

			avgMove += avgMoveArr[j];
			avgEnergy += avgEnergyArr[j];
		}

		avgMove /= _grid->numVerts();
		avgEnergy /= _grid->numVerts();

		cout << i << ": Max move= " << maxMove << ", avgMove: " << avgMove << ", maxEnergy: " << maxEnergy << ", avgEnergy: " << avgEnergy << "\n";

		_reporter->report(*this, "grid_verts_changed");

#if DUMP_OBJ
		if (!filename.empty() && (i % 5) == 0) {
			string str;

			str = filename + "_" + to_string(i);
			_dumpObj.write(str);

			str = filename + "Reduced_" + to_string(i);
			_dumpObj.write(str, 2, CLAMP_VERT | CLAMP_EDGE | CLAMP_TRI);
		}

		if (maxMove < 0.001 * _params.calcMaxEdgeLength())
			break;
#endif
	}

	_grid->rebuildVertTree();
}

void CMesher::splitCells(int numSplits) {
	for (int i = 0; i < numSplits; i++) {
		// TODO make the a CSplitter method
		CSplitter div(*_grid);
		div.splitAll();
	}
}

double CMesher::calVertEnergy(size_t vertIdx) const {
	const auto& vert = _grid->getVert(vertIdx);

	return 0;
}

void CMesher::divideMesh(int numDivisions) {
	int numDivs = _params.calcMaxDivisions();

	for (int i = 0; i < numDivisions; i++) {
		int mask = CLAMP_VERT | CLAMP_EDGE | CLAMP_TRI | CLAMP_FIXED;

		_grid->iterateVerts([&](size_t vertIdx)->bool {
			_grid->clampVertexToCellEdgeCenter(vertIdx);
			return true;
		});

		splitCells(1);

#if DUMP_OBJ
		string splitName("divSplit_");
		splitName += std::to_string(i);
		_dumpObj.write(splitName, 0);
		_dumpObj.write(splitName + "Reduced1", 1, mask);
		_dumpObj.write(splitName + "Reduced2", 2, mask);
#endif
		minimizeMesh(25, -1);

#if DUMP_OBJ
		string minName("divMin_");
		minName += std::to_string(i);
		_dumpObj.write(minName, 0);
		_dumpObj.write(minName + "Reduced1", 1, mask);
		_dumpObj.write(minName + "Reduced2", 2, mask);
#endif
	}
}

struct CMesher::PolylineRec {
	set<TriMesh::CEdge> _edges;
};

struct CMesher::CellPolylineRec {
	inline void addEdge(const TriMesh::CEdge& edge, size_t polylineNum) {
		auto iter = _polylines.find(polylineNum);
		if (iter == _polylines.end()) {
			iter = _polylines.insert(make_pair(polylineNum, PolylineRec())).first;
		}
		iter->second._edges.insert(edge);
	}
	map<size_t, PolylineRec> _polylines;
};

struct CMesher::CellMeshRec {
	inline void addEdge(const TriMesh::CEdge& edge, size_t meshIdx, size_t polylineNum) {
		auto iter = _meshPolylines.find(meshIdx);
		if (iter == _meshPolylines.end()) {
			iter = _meshPolylines.insert(make_pair(meshIdx, CellPolylineRec())).first;
		}
		iter->second.addEdge(edge, polylineNum);
	}
	map<size_t, CellPolylineRec> _meshPolylines;
};
	
template<class FUNC>
void CMesher::iterateCellMeshRecMap(map<size_t, CellMeshRec>& cellSegs, FUNC func) {
	for (const auto& iter0 : cellSegs) {
		bool exit = false;
		size_t cellIdx = iter0.first;
		const auto& mpl = iter0.second._meshPolylines;
		if (mpl.size() != 1) {
			cout << "Unsupported condition\n";
			continue;
		}
		size_t meshIdx = mpl.begin()->first;
		const auto& pl = mpl.begin()->second._polylines;
		if (pl.size() != 1) {
			cout << "Unsupported condition\n";
			continue;
		}

		for (auto iter1 : pl) {
			exit = !func(meshIdx, cellIdx, iter1.first, iter1.second);
			if (exit)
				break;
		}
		if (exit)
			break;
	}
}

struct CornerDistRec {
	double _dist = DBL_MAX;
	CellVertPos _corner = CVP_UNKNOWN;
	Vector3d _point;
	TriMesh::CEdge _edge;

	inline bool operator < (const CornerDistRec& rhs) const {
		return _dist < rhs._dist;
	}
};

class SortPlBySize {
public:
	inline bool operator()(const TriMesh::CPolyLine& lhs, const TriMesh::CPolyLine& rhs) const {
		return lhs.getVerts().size() > rhs.getVerts().size();
	}
};

void CMesher::putCornersOnSharpEdges() {
	int count = 0;
	set<size_t> cellsToSplit;
	for (size_t meshIdx = 0; meshIdx < _modelPtrs.size(); meshIdx++) {
		const CModelPtr& modelPtr = _modelPtrs[meshIdx];
		sort(modelPtr->_polyLines.begin(), modelPtr->_polyLines.end(), SortPlBySize());
		for (size_t polylineNum = 0; polylineNum < modelPtr->_polyLines.size(); polylineNum++) {
			const TriMesh::CPolyLine& pl = modelPtr->_polyLines[polylineNum];
			if (_params.bounds.intersects(pl.calcBoundingBox(*modelPtr))) {
				CPolylineFitter fitter(*this, meshIdx, polylineNum);
				if (fitter.doFit(cellsToSplit) > 0) {
					count++;
					if (count >= 3)
						break;
				}
			}
		}
	}

	if (!_grid->verify())
		cout << "Bad mesh after polyline fit\n";

#if DUMP_OBJ
	_dumpObj.write ("alignedMeshPreSplitPreMin");
	_dumpObj.write("alignedMeshPreSplitPreMinReduced", 1, CLAMP_VERT | CLAMP_EDGE | CLAMP_TRI);
#endif

	minimizeMesh(25, -1);

#if DUMP_OBJ
	_dumpObj.write("alignedMeshPostSplitPreMin");
	_dumpObj.write("alignedMeshPostSplitPreMinReduced", 1, CLAMP_VERT | CLAMP_EDGE | CLAMP_TRI);
#endif

	{
		vector<size_t> cells;
		cells.insert(cells.end(), cellsToSplit.begin(), cellsToSplit.end());

#if DUMP_OBJ
		_dumpObj.writeCells("cellToBeSplit", cells);
#endif

	}

	CSplitter divider(*_grid);
	divider.splitCells(cellsToSplit);
	const auto& newCells = divider.getNewCells();

	for (size_t cellId : newCells) {
		const auto& cell = _grid->getCell(cellId);
		for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
			_grid->clampVertex(cell.getVertIdx(p));
		}
	}

#if DUMP_OBJ
	_dumpObj.writeCells("cellFromSplit", newCells);
	_dumpObj.writeCells("clampedCells", divider.getClampedCells());
#endif

	if (!_grid->verify())
		cout << "Bad mesh after diagonal split\n";
}

void CMesher::runStat(CMesher* self) {
	self->run();
}

void CMesher::runAsThread() {
	_thread  = new thread(&runStat, this);
}

void CMesher::init() {
#if 0
	_params.minGapSize = findMinimumGap();
#else
	_params.minGapSize = 0.00848194;
#endif

	cout << "Minimum normal gap : " << _params.minGapSize << " m (" << (_params.minGapSize * 39.37) << " in)\n";
	cout << "Max edge length    : " << _params.calcMaxEdgeLength() << " m \n";
	cout << "Min edge length    : " << _params.calcMinEdgeLength() << " m \n";
	cout << "Max divisions      : " << _params.calcMaxDivisions() << "\n";
	cout << "Sharp angle        : " << _params.sharpAngleDeg << " deg\n";

#if DUMP_OBJ
	dumpModelObj("model");
	const double sinEdgeAngle = sin(_params.sharpAngleDeg * EIGEN_PI / 180.0);
	dumpModelSharpEdgesObj("sharp edges", sinEdgeAngle);
#endif
	_grid->setBounds(_params.bounds);
}

void CMesher::makeInitialGrid() {
	_grid->init(_params.bounds);
	clampBoundaries();
	//					splitCellsAroundPolylines();
	//					_dumpObj.write("polylineSplit");
	snapToCusps();
#if DUMP_OBJ
	_dumpObj.write("cuspSnap");
#endif

	for (int i = 0; i < 0; i++) {
		// TODO make the a CSplitter method
		CSplitter div(*_grid);
		div.splitAll();
	}

#if DUMP_OBJ
	_dumpObj.write("postSplit");
#endif

	save(savePath + "initial.grid");
}

ErrorCode CMesher::run() {
	try {
		init();

		if (!read(savePath + "postFit.grid")) {
			if (!read(savePath + "preFit.grid")) {
				if (!read(savePath + "initial.grid")) {
					makeInitialGrid();
				}
				_reporter->report(*this, "grid_topol_change");
//				return NO_ERR;
				minimizeMesh(50, -1);

#if DUMP_OBJ
				_dumpObj.write("preFit", 0);
				_dumpObj.write("preFitReduced1", 1, CLAMP_VERT | CLAMP_EDGE | CLAMP_TRI);
				_dumpObj.write("preFitReduced2", 2, CLAMP_VERT | CLAMP_EDGE);
#endif

				save(savePath + "preFit.grid");
				cout << "Saved state to preFit.grid\n";
			}
			cout.flush();
			_reporter->report(*this, "grid_topol_change");

			putCornersOnSharpEdges();
			// Need to save EVERYTHING!! The polyline numbers are different after reading!
			save(savePath + "postFit.grid");
#if DUMP_OBJ
			_dumpObj.write("postFit", 0);
			_dumpObj.write("postFitReduced1", 1, CLAMP_VERT | CLAMP_EDGE | CLAMP_TRI);
			_dumpObj.write("postFitReduced2", 2, CLAMP_VERT | CLAMP_EDGE);
			_dumpObj.writeFaces("clampedFaces", 2, CLAMP_VERT | CLAMP_EDGE | CLAMP_FIXED);
			_dumpObj.writeFaces("bounds", 4, CLAMP_PARALLEL | CLAMP_PERPENDICULAR | CLAMP_FIXED);
#endif
		}
		_reporter->report(*this, "grid_topol_change");

		GridVert::clearHistory();
		minimizeMesh(25, -1);
		GridVert::writeHistory(savePath + "vertHistory.csv");

#if DUMP_OBJ
		_dumpObj.write("alignedMeshMin", 0);
		_dumpObj.write("alignedMeshMinReduced1", 1, CLAMP_VERT | CLAMP_EDGE | CLAMP_TRI);
		_dumpObj.write("alignedMeshMinReduced2", 2, CLAMP_VERT | CLAMP_EDGE);
		_dumpObj.writeFaces("alignedFacesMinReduced1", 1, CLAMP_VERT | CLAMP_EDGE | CLAMP_TRI);
		_dumpObj.writeFaces("alignedFacesMinReduced2", 2, CLAMP_VERT | CLAMP_EDGE);
#endif

		divideMesh(1);

	}
	catch (StopException) {
		return NO_ERR;
	}
	return NO_ERR;
}

void CMesher::dumpModelObj(const string& filenameRoot) const {
	if (!_modelPtrs.empty()) {
		string filename(filenameRoot + ".obj");
		ofstream out(dumpPath + filename);
		_modelPtrs[0]->dumpObj(out);
	}
}

void CMesher::dumpModelSharpEdgesObj(const string& filenameRoot, double sinAngle) const {
	if (!_modelPtrs.empty()) {
		string filename(filenameRoot + ".obj");
		ofstream out(dumpPath + filename);
		_modelPtrs[0]->dumpModelSharpEdgesObj(out, sinAngle);
	}
}

void CMesher::Reporter::report(const CMesher& mesher, const std::string& key) {

}

void CMesher::Reporter::reportModelAdded(const CMesher& mesher, const CModelPtr& model) {

}

bool CMesher::Reporter::isRunning() const {
	return true;
}
