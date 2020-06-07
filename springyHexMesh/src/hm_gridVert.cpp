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
#include <iomanip>
#include  <mutex>

#include <tm_math.h>
#include <hm_gridVert.h>
#include <hm_grid.h>

namespace HexahedralMesher {

	using namespace std;

	static const thread::id main_thread_id = this_thread::get_id();
	thread_local int _thNum = 0;
	thread_local int _thIdx = 0;

	template<int NUM_THREADS>
	void GridVertTempl<NUM_THREADS>::setThreadNumber(int threadNumber) {
		if (main_thread_id == this_thread::get_id())
			throw "Don't change the values for the main thread!";
		_thNum = threadNumber;
		_thIdx = _thNum + 1;
	}

	template<int NUM_THREADS>
	int GridVertTempl<NUM_THREADS>::getThreadNumber() {
		return _thNum;
	}

	template<int NUM_THREADS>
	const Vector3d& GridVertTempl<NUM_THREADS>::getPt() const {
		return _pt[_thIdx];
	}

	template<int NUM_THREADS>
	Vector3d& GridVertTempl<NUM_THREADS>::getPt() {
		return _pt[_thIdx];
	}

	template<int NUM_THREADS>
	inline void GridVertTempl<NUM_THREADS>::setPoint(const Vector3d& pt) {
		checkNAN(pt);
		_pt[_thIdx] = pt;
		_changeNumber++;
	}

	template<int NUM_THREADS>
	void GridVertTempl<NUM_THREADS>::setClamp(const GridBase& grid, const TopolRef& clamp) {
		if (!clamp.verify(grid))
			throw "Invald clamp";
		_clampTopol[_thIdx] = clamp;
	}

	template<int NUM_THREADS>
	void GridVertTempl<NUM_THREADS>::copyToThread() {
		// The optimizer reads from the thread's _pt, may write to _stashPt.
		// When the pass is over, _stashPt is written to thread main. _stash values must be copied first.
		_stashPt = _pt[0];
		_stashClamp = _clampTopol[0];

		for (int i = 1; i <= NUM_THREADS; i++) {
			_pt[i] = _pt[0];
			_clampTopol[i] = _clampTopol[0];
		}
	}

	template<int NUM_THREADS>
	void GridVertTempl<NUM_THREADS>::copyFromThread() {
		_pt[0] = _stashPt;
		_clampTopol[0] = _stashClamp;
	}

	template<int NUM_THREADS>
	void GridVertTempl<NUM_THREADS>::removeCellIndex(size_t cellIdx) {
		for (auto iter = _cellIndices.begin(); iter != _cellIndices.end(); iter++) {
			if (*iter == cellIdx) {
				_cellIndices.erase(iter);
				break;
			}
		}

		for (size_t i = 0; i < _cellIndices.size(); i++) {
			if (_cellIndices[i] == cellIdx) {
				throw "should be removed";
			}
		}
	}

	template<int NUM_THREADS>
	ClampType GridVertTempl<NUM_THREADS>::getClampType() const {
		return _clampTopol[_thIdx].getClampType();
	}

	template<int NUM_THREADS>
	const TopolRef& GridVertTempl<NUM_THREADS>::getClamp() const {
		return _clampTopol[_thIdx];
	}

	template<int NUM_THREADS>
	TopolRef& GridVertTempl<NUM_THREADS>::getClamp() {
		return _clampTopol[_thIdx];
	}

	template<int NUM_THREADS>
	ostream& operator << (ostream& os, const vector<size_t>& indices) {
		os << "[";
		for (size_t i = 0; i < indices.size(); i++) {
			os << indices[i];
			if (i < indices.size() - 1)
				os << ", ";
		}
		os << "]";

		return os;
	}

	template<int NUM_THREADS>
	ostream& operator << (ostream& os, const GridVertTempl<NUM_THREADS>& vert) {
		const auto& pt = vert.getPt();
		os << "[" << fixed << setprecision(4) << setw(8) << right << pt[0] << ", " << pt[1] << ", " << pt[2] << "]";
		return os;
	}

	template<int NUM_THREADS>
	bool GridVertTempl<NUM_THREADS>::verify(const GridBase& grid, bool verifyCells) const {
		// Make sure the cells we reference reference this vert
		for (size_t cellId : _cellIndices) {
			if (!grid.cellExists(cellId))
				return false;
			const auto& cell = grid.getCell(cellId);
			if (cell.getVertsPos(_selfIndex) == CVP_UNKNOWN)
				return false;
			if (verifyCells && !cell.verify(grid, false))
				return false;
		}

		if (!_clampTopol->verify(grid))
			return false;

		return true;
	}

	template<int NUM_THREADS>
	void GridVertTempl<NUM_THREADS>::save(ostream& out) const {
		if (_selfIndex == stm1)
			throw "unset vert._selfIndex";
		out << "VT: " << _selfIndex << "\n";
		out << "PT: " << fixed << setprecision(filePrecision) << _pt[0][0] << " " << _pt[0][1] << " " << _pt[0][2] << "\n";
		out << "CI:";
		for (size_t cellIdx : _cellIndices)
			out << " " << cellIdx;
		out << "\n";

		_clampTopol[0].save(out);
		
	}

	template<int NUM_THREADS>
	bool GridVertTempl<NUM_THREADS>::linkedToCell(size_t cellIdx) const {
		for (size_t i = 0; i < _cellIndices.size(); i++) {
			if (_cellIndices[i] == cellIdx)
				return true;
		}
		return false;
	}

	template<int NUM_THREADS>
	size_t GridVertTempl<NUM_THREADS>::getAdjacentCellIndices(const Grid& grid, int numLevels, std::set<size_t>& adjCellIndices) const {
		adjCellIndices.clear();

		// Fill with our cell indices
		adjCellIndices.insert(_cellIndices.begin(), _cellIndices.end());

		for (int i = 0; i < numLevels + 1; i++) {
			// Add the indices from adjacent cells
			set<size_t> adjCellIndicesOrig(adjCellIndices);
			for (size_t cellIdx : adjCellIndicesOrig) {
				const auto& cell = grid.getCell(cellIdx);
				for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
					const auto& adjVert = cell.getVert(grid, p);
					adjCellIndices.insert(adjVert._cellIndices.begin(), adjVert._cellIndices.end());
				}
			}
		}
		return adjCellIndices.size();
	}

	template<int NUM_THREADS>
	size_t GridVertTempl<NUM_THREADS>::getVertEdgeEnd(const Grid& grid, VertEdgeDir edgeDir) const {
		for (CellVertPos pos = LWR_FNT_LFT; pos < CVP_UNKNOWN; pos++) {
			size_t cellIdx = getCellIndex(pos);
			if (cellIdx != stm1) {
				const GridCell& cell = grid.getCell(cellIdx);
				auto vertPos = cell.getVertsPos(_selfIndex);
				size_t otherVertIdx = cell.getVertsEdgeEndVertIdx(vertPos, edgeDir);
				if (otherVertIdx != stm1)
					return otherVertIdx;
			}
		}
		return stm1;
	}

	template<int NUM_THREADS>
	size_t GridVertTempl<NUM_THREADS>::getVertEdgeEndIndices(const Grid& grid, std::set<size_t>& edgeEnds) const {
		edgeEnds.clear();
		for (CellVertPos cn = LWR_FNT_LFT; cn < CVP_UNKNOWN; cn++) {
			const auto& cell = grid.getCell(getCellIndex(cn));
			auto vertPos = cell.getVertsPos(_selfIndex);
			for (VertEdgeDir ed = X_POS; ed < VED_UNKNOWN; ed++) {
				size_t vertIdx = cell.getVertsEdgeEndVertIdx(vertPos, ed);
				edgeEnds.insert(vertIdx);
			}
		}

		return edgeEnds.size();
	}

	template<int NUM_THREADS>
	double GridVertTempl<NUM_THREADS>::findVertMinAdjEdgeLength(const Grid& grid) const {
		const Vector3d& pt0 = getPt();

		set<size_t> checked;
		double minLength = DBL_MAX;
		for (int i = 0; i < getNumCells(); i++) {
			const GridCell& cell = grid.getCell(getCellIndex(i));
			CellVertPos p0 = cell.getVertsPos(_selfIndex);
			CellVertPos adjEnds[3];
			GridCell::getAdjacentEdgeEnds(p0, adjEnds);
			for (int j = 0; j < 3; j++) {
				CellVertPos p1 = adjEnds[j];
				size_t vertIdx1 = cell.getVertIdx(p1);
				if (checked.count(vertIdx1) != 0)
					continue;
				checked.insert(vertIdx1);
				const Vector3d& pt1 = grid.getVert(vertIdx1).getPt();
				double l = (pt1 - pt0).norm();
				if (l < minLength)
					minLength = l;
			}
		}
		return minLength;
	}

	template<int NUM_THREADS>
	double GridVertTempl<NUM_THREADS>::calcDegreesOfFreedomMetric(const Grid& grid) const {
		/*
		Add all the edge direction vectors for this vertex. If the sum is _small_ the vertex is has well balanced degrees
		of freedom. 
		
		Clamping changes this. Clamped verts get a metric of zero, degrees of freedom balanced.
		
		*/

		if (!getClamp().matches(CLAMP_NONE))
			return 0;

		vector<Vector3d> edgeDirs;
		double avgEdgeLength = 0;

		/*
		Collect edge direction vectors which ARE NOT close to parallel to another in the set.
		for a 6 edge, orthoganal case this yields 6 opposed vectors.
		*/
		for (size_t cellId : _cellIndices) {
			const auto& cell = grid.getCell(cellId);
			size_t adjVerts[3];
			cell.getVertsEdgeIndices(cell.getVertsPos(_selfIndex), adjVerts);
			for (int i = 0; i < 3; i++) {
				Vector3d v0 = (getPt() - grid.getVert(adjVerts[i]).getPt());
				double len = v0.norm();
				v0 /= len;
				bool found = false;
				for (const Vector3d& v1 : edgeDirs) {
					double dp = fabs(v0.dot(v1));
					if ((1 - dp) < 0.01) {
						found = true;
						break;
					}
				}

				if (!found) {
					avgEdgeLength += len;
					edgeDirs.push_back(v0);
				}
			}
		}

		avgEdgeLength /= edgeDirs.size();
		Vector3d sum(0, 0, 0);
		for (const Vector3d& dir : edgeDirs) {
			sum += dir;
		}
		sum /= (double)edgeDirs.size();
		double metric = sum.norm() / avgEdgeLength;
		return metric;
	}

#define LOG_HISTORY 0

#if LOG_HISTORY
	static mutex vertHistoryLock;
	static map<ClampType, map<size_t, vector<double>>> vertHistory;
#endif

	template<int NUM_THREADS>
	void GridVertTempl<NUM_THREADS>::clearHistory() {
#if LOG_HISTORY
		vertHistory.clear();
#endif
	}

	template<int NUM_THREADS>
	void GridVertTempl<NUM_THREADS>::writeHistory(const std::string& path) {
#if LOG_HISTORY

		map<ClampType, vector<double>> typeHistory;
		for (auto& iter : vertHistory) {
			ClampType clampType = iter.first;
			auto& hist = iter.second;

			vector<double> avg;
			size_t row = 0;
			bool done = false;
			double sum2 = 0;
			while (!done) {
				done = true;
				double sum = 0;
				for (const auto& entry : hist) {
					if (row < entry.second.size()) {
						done = false;
						sum += entry.second[row];
					}
				}
				sum = sum / hist.size();
				avg.push_back(sum);
				sum2 += sum;
				row++;
			}

			if (sum2 > 0)
				typeHistory[clampType] = avg;
		}

		vertHistory.clear();

		ofstream out(path);
		for (auto& iter : typeHistory) {
			ClampType ct = iter.first;
			out << ct << " ";
		}
		out << "\n";

		size_t row = 0;
		bool done = false;
		while (!done) {
			done = true;
			for (const auto& entry : typeHistory) {
				const auto& vals = entry.second;
				if (row < vals.size()) {
					done = false;
					out << vals[row] << " ";
				}
			}
			row++;
			out << "\n";
		}
#endif
	}

	template<int NUM_THREADS>
	void GridVertTempl<NUM_THREADS>::moveAuxToPrimary() {
		if (_clampTopol[0].getClampType() != _clampTopol[_thIdx].getClampType())
			throw "AuxClampType != ClampType";

		double delta = (_pt[_thIdx] - _pt[0]).norm();

		_pt[0] = _pt[_thIdx];
		_clampTopol[0] = _clampTopol[_thIdx];
		_changeNumber++;

#if LOG_HISTORY
		{
			lock_guard<mutex> lock(vertHistoryLock);
			vertHistory[_clampTopol[0].getClampType()][_selfIndex].push_back(delta);
		}
#endif
	}

	template<int NUM_THREADS>
	void GridVertTempl<NUM_THREADS>::clearAuxData() {
		for (int i = 1; i < NUM_THREADS + 1; i++) {
			_pt[i] = _pt[0];
			_clampTopol[i] = _clampTopol[0];
		}
	}

	template<int NUM_THREADS>
	void GridVertTempl<NUM_THREADS>::dump(ostream& out, std::string pad) const {
		out << pad << "pt      : " << *this << "\n";
		out << pad << "cells    : {";
		for (int i = 0; i < 8; i++) {
			out << _cellIndices[i] << "\n";
		}
		out << "}\n";
	}

	template<int NUM_THREADS>
	bool GridVertTempl<NUM_THREADS>::readVersion1(std::istream& in) {
		string str;

		in >> str >> _selfIndex;
		if (str != "VT:") return false;

		in >> str >> _pt[0][0] >> _pt[0][1] >> _pt[0][2];
		if (str != "PT:") return false;
		for (auto& p : _pt)
			p = _pt[0];

		in >> str;
		if (str != "CI:") return false;
		while (in.peek() != '\n') {
			size_t val;
			in >> val;
			_cellIndices.push_back(val);
		}

		if (!_clampTopol[0].readVersion1(in)) return false;
		for (auto& ct : _clampTopol)
			ct = _clampTopol[0];

		_changeNumber++;

		return true;
	}

	template class GridVertTempl<8>;

	template std::ostream& operator << (std::ostream& os, const GridVertTempl<8>& vert);

}
