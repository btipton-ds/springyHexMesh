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
#include <vector>
#include <list>
#include <map>
#include <iostream>

#ifdef __linux__
#include <pthread.h>
#include <thread>
#else
#include <thread>
#endif


#include <tm_spatialSearch.h>
#include <hm_types.h>
#include <hm_topolRef.h>
#include <hm_gridCell.h>
#include <hm_gridVert.h>

namespace HexahedralMesher {

	class GridBase {
		template<typename FUNC> friend struct IterVertThread;

		static void setThreadNumber(int threadNumber);

	public:
		using SearchTree = CSpatialSearchST<BoundingBox>;

		static int getThreadNumber();

		GridBase();

		void setBounds(const BoundingBox& bbox);
		void clear();

		void save(std::ostream& out) const;
		bool read(std::istream& in);
		void save(const std::string& pathname) const;
		bool read(const std::string& pathname);

		size_t add(const Vector3d& pt);
		bool vertExists(size_t vertIdx) const;
		const GridVert& getVert(size_t idx) const;
		GridVert& getVert(size_t idx);
		bool setVertPos(size_t vertIdx, const Vector3d& pt);
		std::vector<size_t> findVerts(const BoundingBox& bb) const;
		size_t findVerts(const BoundingBox& bb, std::vector<size_t>& cellIndices) const;
		void rebuildVertTree() const;
		size_t numVerts() const;
		size_t numClampedVerts() const;

		size_t numCells() const;
		bool cellExists(size_t cellIdx) const;
		const GridCell& getCell(size_t idx) const;
		GridCell& getCell(size_t idx);
		size_t addCell(const GridCell& cell);
		void deleteCell(size_t cellIdx);

		void clearSearchTrees();

		bool verify() const;
		bool verifyVertCount(size_t delta = 0) const;

		template <typename FUNC>
		void iterateCells(FUNC func);

		template <typename FUNC>
		void iterateCells(FUNC func) const;

		template <typename FUNC>
		void iterateVerts(FUNC func, int numCores = 1);

		template <typename FUNC>
		void iterateVerts(FUNC func, int numCores = 1) const;

		void dumpText(std::ostream& out) const;

	private:
		bool readVersion1(std::istream& in);

		size_t add(const GridVert& vert);

		std::vector<GridVert> _verts;

		std::vector<size_t> _cellIndexMap;
		std::vector<GridCell> _cellStorage;

		mutable SearchTree _vertTree;
	};

	inline void GridBase::setThreadNumber(int threadNumber) {
		GridVert::setThreadNumber(threadNumber);
	}

	inline int GridBase::getThreadNumber() {
		return GridVert::getThreadNumber();
	}

	inline size_t GridBase::numVerts() const {
		return _verts.size();
	}

	inline bool GridBase::vertExists(size_t vertIdx) const {
		return vertIdx < _verts.size();
	}

	inline const GridVert& GridBase::getVert(size_t idx) const {
		return _verts[idx];
	}

	inline GridVert& GridBase::getVert(size_t idx) {
		return _verts[idx];
	}

	inline bool GridBase::cellExists(size_t cellIdx) const {
		return _cellIndexMap[cellIdx] != stm1;
	}

	inline const GridCell& GridBase::getCell(size_t cellIdx) const {
		return _cellStorage[_cellIndexMap[cellIdx]];
	}

	inline GridCell& GridBase::getCell(size_t cellIdx) {
		return _cellStorage[_cellIndexMap[cellIdx]];
	}

	inline size_t GridBase::numCells() const {
		return _cellIndexMap.size();
	}

	template <typename FUNC>
	inline void GridBase::iterateCells(FUNC func) {
		// Don't extend the loop if cells are added to the list
		size_t numCells = _cellIndexMap.size();
		for (size_t cellIdx = 0; cellIdx < numCells; cellIdx++) {
			size_t cellStorageIdx = _cellIndexMap[cellIdx];
			if (cellStorageIdx != stm1) {
				if (!func(cellIdx))
					break;
			}
		}
	}

	template <typename FUNC>
	inline void GridBase::iterateCells(FUNC func) const {
		// Don't extend the loop if cells are added to the list
		size_t numCells = _cellIndexMap.size();
		for (size_t cellIdx = 0; cellIdx < numCells; cellIdx++) {
			size_t cellStorageIdx = _cellIndexMap[cellIdx];
			if (cellStorageIdx != stm1) {
				if (!func(cellIdx))
					break;
			}
		}
	}

	template<typename FUNC>
	struct IterVertThread {
	private:
		static void run(void* self)
		{
			((IterVertThread*)self)->runSelf();
		}

		void runSelf() {
			GridBase::setThreadNumber(_threadNum); // Entry zero is for primary storage
			for (size_t vertIdx = _start; vertIdx < _end; vertIdx++) {
				if (vertIdx % _numThreads == _threadNum)
					_func(vertIdx);
			}
		}
	public:
		IterVertThread(int numThreads, int threadNum, FUNC func, size_t start, size_t end)
			: _numThreads(numThreads)
			, _threadNum(threadNum)
			, _func(func)
			, _start(start)
			, _end(end)
			, _thread(run, (void*)this)
		{}

		std::thread& getThread() {
			return _thread;
		}

		std::thread _thread;
		int _numThreads, _threadNum;
		size_t _start, _end;
		FUNC _func;
	};

	template <typename FUNC>
	inline void GridBase::iterateVerts(FUNC func, int numCores) {
		if (numCores < 2) {
			for (size_t i = 0; i < _verts.size(); i++) {
				if (!func(i))
					break;
			}
		}
		else {
			std::vector<std::shared_ptr<IterVertThread<FUNC>>> threads;
			for (int i = 0; i < numCores; i++) {
				std::shared_ptr<IterVertThread<FUNC>> threadPtr = std::make_shared<IterVertThread<FUNC>>(numCores, i, func, 0, _verts.size());
				threads.push_back(threadPtr);
			}

			for (auto& thread : threads) {
				thread->getThread().join();
			}
		}
	}

	template <typename FUNC>
	inline void GridBase::iterateVerts(FUNC func, int numCores) const {
		if (numCores < 2) {
			for (size_t i = 0; i < _verts.size(); i++) {
				if (!func(i))
					break;
			}
		} else {
			std::vector<std::shared_ptr<IterVertThread<FUNC>>> threads;
			size_t steps = _verts.size() / numCores + 1;
			size_t start = 0;
			for (int i = 0; i < numCores; i++) {
				size_t end = start + steps;
				if (end > _verts.size())
					end = _verts.size();
				std::shared_ptr<IterVertThread<FUNC>> threadPtr = std::make_shared<IterVertThread<FUNC>>(numCores, i, func, start, end);
				threads.push_back(threadPtr);
				start = end;
			}

			for (auto& thread : threads) {
				thread->getThread().join();
			}
		}
	}

}
