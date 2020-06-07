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

#include <iostream>
#include <vector>
#include <set>

#include <hm_types.h>
#include <hm_topolRef.h>
#include <hm_gridFace.h>

namespace HexahedralMesher {

	template<int NUM_THREADS = 8>
	class GridVertTempl {
		friend class GridBase;
		static void setThreadNumber(int threadNumber);
	public:
		static int getThreadNumber();

		static void clearHistory();
		static void writeHistory(const std::string& path);

		GridVertTempl(size_t selfIndex = stm1, int numThreads = 8);
		GridVertTempl(size_t selfIndex, const Vector3d& pt, int numThreads = 8);
		GridVertTempl(const GridVertTempl& src) = default;
		bool verify(const GridBase& grid, bool verifyCells = false) const;
		void save(std::ostream& out) const;

		size_t getIndex() const;
		size_t getChangeNumber() const;
		void addCellIndex(size_t cellIdx);
		void removeCellIndex(size_t cellIdx);
		bool linkedToCell(size_t cellIdx) const;
		size_t getNumCells() const;
		size_t getCellIndex(size_t i) const;
		const std::vector<size_t>& getCellIndices() const;
		// Num levels == 0 gets only the immediately adjacent cells
		size_t getAdjacentCellIndices(const Grid& grid, int numLevels, std::set<size_t>& adjCellIndices) const;
		size_t getVertEdgeEndIndices(const Grid& grid, std::set<size_t>& edgeEnds) const;

		void setPoint(const Vector3d& pt);
		const Vector3d& getPt() const;
		Vector3d& getPt();

		ClampType getClampType() const;
		const TopolRef& getClamp() const;
		TopolRef& getClamp();
		void setClamp(const GridBase& grid, const TopolRef& clamp);

		void copyToThread();
		void copyFromThread();

		void setStashPoint(const Vector3d& pt);
		const Vector3d& getStashPt() const;
		const TopolRef& getStashClamp() const;
		void setStashClamp(const GridBase& grid, const TopolRef& clamp);

		double calcDegreesOfFreedomMetric(const Grid& grid) const;
		size_t getVertEdgeEnd(const Grid& grid, VertEdgeDir edgeDir) const;
		double findVertMinAdjEdgeLength(const Grid& grid) const;

		void moveAuxToPrimary();
		void clearAuxData();

		void dump(std::ostream& out, std::string pad) const;
	private:
		friend class GridBase;
		friend class Grid;

		bool readVersion1(std::istream& in);

		size_t _selfIndex;
		// Index 0 is 'non-threaded' 1 - numThreads + 1 are the thread copies
		Vector3d _pt[NUM_THREADS + 1];
		Vector3d _stashPt;

		TopolRef _clampTopol[NUM_THREADS + 1];
		TopolRef _stashClamp;

		size_t _changeNumber = 0;
		std::vector<size_t> _cellIndices;
	};

	template<int NUM_THREADS>
	std::ostream& operator << (std::ostream& os, const GridVertTempl<NUM_THREADS>& vert);

	template<int NUM_THREADS>
	inline GridVertTempl<NUM_THREADS>::GridVertTempl(size_t selfIndex, int numThreads)
		: _selfIndex(selfIndex)
		, _changeNumber(0)
	{
		_pt[0] = Vector3d(DBL_MAX, DBL_MAX, DBL_MAX);
	}

	template<int NUM_THREADS>
	inline GridVertTempl<NUM_THREADS>::GridVertTempl(size_t selfIndex, const Vector3d& pt, int numThreads)
		: _selfIndex(selfIndex)
	{
		_pt[0] = pt;
		_changeNumber++;
	}

	template<int NUM_THREADS>
	inline size_t GridVertTempl<NUM_THREADS>::getIndex() const {
		return _selfIndex;
	}

	template<int NUM_THREADS>
	inline size_t GridVertTempl<NUM_THREADS>::getChangeNumber() const {
		return _changeNumber;
	}

	template<int NUM_THREADS>
	inline size_t GridVertTempl<NUM_THREADS>::getNumCells() const {
		return _cellIndices.size();
	}

	template<int NUM_THREADS>
	inline size_t GridVertTempl<NUM_THREADS>::getCellIndex(size_t i) const {
		return _cellIndices[i];
	}

	template<int NUM_THREADS>
	inline const std::vector<size_t>& GridVertTempl<NUM_THREADS>::getCellIndices() const {
		return _cellIndices;
	}
	
	template<int NUM_THREADS>
	inline void GridVertTempl<NUM_THREADS>::addCellIndex(size_t cellIdx) {
		for (size_t i = 0; i < _cellIndices.size(); i++) {
			if (_cellIndices[i] == cellIdx)
				return;
		}
		_cellIndices.push_back(cellIdx);
	}

	template<int NUM_THREADS>
	inline void GridVertTempl<NUM_THREADS>::setStashPoint(const Vector3d& pt) {
		_stashPt = pt;
	}

	template<int NUM_THREADS>
	inline const Vector3d& GridVertTempl<NUM_THREADS>::getStashPt() const {
		return _stashPt;
	}

	template<int NUM_THREADS>
	inline const TopolRef& GridVertTempl<NUM_THREADS>::getStashClamp() const {
		return _stashClamp;
	}

	template<int NUM_THREADS>
	inline void GridVertTempl<NUM_THREADS>::setStashClamp(const GridBase& grid, const TopolRef& clamp) {
		if (!clamp.verify(grid))
			throw "Invald clamp";
		_stashClamp = clamp;
	}

	using GridVert = GridVertTempl<8>;
}
