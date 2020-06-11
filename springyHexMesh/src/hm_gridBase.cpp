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

#include <hm_gridBase.h>

#include <set>
#include <iostream>
#include <fstream>

#include <hm_tables.h>
#include <hm_gridVert.h>
#include <hm_gridEdge.h>
#include <hm_dump.h>
#include <meshProcessor.h>

namespace HexahedralMesher {
	using namespace std;

	GridBase::GridBase()
	{}

	void GridBase::setBounds(const BoundingBox& bbox) {
		BoundingBox searchBBox(bbox);
		auto r = bbox.range();
		searchBBox.grow(0.01 * r.norm());
		_vertTree.reset(searchBBox);
	}

	void GridBase::clear() {
		_verts.clear();
		_cellIndexMap.clear();
		_cellStorage.clear();
		_vertTree.clear();;
	}

	void GridBase::save(std::ostream& out) const {
		out << "GridBase version 1\n";

		out << "Verts " << _verts.size() << "\n";
		for (const auto& vert : _verts)
			vert.save(out);
		
		out << "CellIndexMap " << _cellIndexMap.size() << "\n";
		for (size_t i = 0; i < _cellIndexMap.size(); i++) {
			if (_cellIndexMap[i] != stm1) {
				out << i << " " << _cellIndexMap[i] << "\n";
			}
		}

		out << "Cells " << _cellStorage.size() << "\n";
		for (const auto& cell : _cellStorage)
			cell.save(out);

	}

	bool GridBase::read(istream& in) {
		string str1, str2;
		int version;
		in >> str1 >> str2 >> version;
		if (str1 == "GridBase" && str2 == "version") {
			if (version == 1)
				return readVersion1(in);
		}
		return false;
	}

	void GridBase::save(const std::string& pathname) const {
		ofstream out(pathname);
		save(out);
	}

	bool GridBase::read(const std::string& pathname) {
		ifstream in(pathname);
		if (in.good())
			return read(in);
		return false;
	}

	size_t GridBase::add(const Vector3d& pt) {
		size_t selfIndex = _verts.size();
		return add(GridVert(selfIndex, pt));
	}

	size_t GridBase::add(const GridVert& vert) {
		const auto& pt = vert.getPt();
		BoundingBox bb(pt);
		vector<size_t> hits;
		_vertTree.find(bb, hits);
		for (size_t hit : hits) {
			if (tolerantEquals(_verts[hit].getPt(), pt)) {
				return hit;
			}
		}
		size_t result = _verts.size();
		_verts.push_back(vert);

		// We need the vertex tree for building the disorganized mesh.
		_vertTree.add(bb, result);
		verifyVertCount();
		return result;
	}

	size_t GridBase::addCell(const GridCell& cell) {
		size_t cellId = _cellIndexMap.size();
		size_t cellStorageIdx = _cellStorage.size();
		_cellStorage.push_back(cell);
		_cellIndexMap.push_back(cellStorageIdx);
		GridCell& newCell = getCell(cellId);
		newCell._id = cellId;
		newCell.attach(*this);

		if (!newCell.verify(*this, true)) {
			throw "New cell is invald";
		}
		return cellId;
	}

	void GridBase::deleteCell(size_t cellId) {
		size_t emptyStorageIdx = _cellIndexMap[cellId];
		if (emptyStorageIdx == stm1)
			return;

		size_t verts[8];
		GridCell& dead = getCell(cellId);
		for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++)
			verts[p] = dead.getVertIdx(p);

		dead.detach(*this);
		_cellIndexMap[cellId] = stm1;

		if (_cellStorage.size() > 1) {
			// Repoint the entry pointing to the back to the empty entry
			size_t oldStorageIdx = _cellStorage.size() - 1;
			for (size_t mapIdx = 0; mapIdx < _cellIndexMap.size(); mapIdx++) {
				if (_cellIndexMap[mapIdx] == oldStorageIdx) {
					_cellIndexMap[mapIdx] = emptyStorageIdx;
					break;
				}
			}
			// move the back to the empty entry
			_cellStorage[emptyStorageIdx] = _cellStorage.back();

			// pop the back
			_cellStorage.pop_back();

			for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
				const auto& vert = getVert(verts[p]);
				if (!vert.verify(*this)) {
					throw "Bad vert after deleting cell";
				}
			}

		}
#if 0
		if (!verify()) {
			throw "deleteCell broke the grid.";
		}
#endif
	}

	bool GridBase::setVertPos(size_t vertIdx, const Vector3d& pt) {
		GridVert& vert = _verts[vertIdx];
		vector<Vector3d> badPts;
		badPts.push_back(vert.getPt());
		if (!verifyVertCount())
			throw "Search tree size mismatch prior to removal.";
		BoundingBox bb;
		bb.merge(vert.getPt());
		if (!_vertTree.remove(bb, vertIdx))
			throw "vertex not in search tree.";

		vert.setPoint(pt);

		bb.clear();
		bb.merge(vert.getPt());
		if (!_vertTree.add(bb, vertIdx))
			throw "Failed to add vertex to search tree.";
		if (!verifyVertCount())
			throw "Search tree size mismatch after replacement.";

		vector<size_t> vertIndices = _vertTree.find(bb);
		bool found = false;
		for (size_t i : vertIndices) {
			found = found || i == vertIdx;
		}
		if (!found) {
			throw "Could not find the point after replacement.";
		}
		_verts[vertIdx].verify(*this, vertIdx);
		return true;
	}

	void GridBase::clearSearchTrees() {
		_vertTree.clear();
	}

	vector<size_t> GridBase::findVerts(const BoundingBox& bb) const {
		vector<size_t> cellIndices;
		findVerts(bb, cellIndices);
		return cellIndices;
	}

	size_t GridBase::findVerts(const BoundingBox& bb, vector<size_t>& cellIndices) const {
		if (_vertTree.numInTree() == 0) {
			rebuildVertTree();
		}
		_vertTree.find(bb, cellIndices);
		return cellIndices.size();
	}

	void GridBase::rebuildVertTree() const {
		_vertTree.clear();
		for (size_t vertIdx = 0; vertIdx < _verts.size(); vertIdx++) {
			const auto& pt = _verts[vertIdx].getPt();
			_vertTree.add(BoundingBox(pt), vertIdx);
		}
	}

	bool GridBase::verify() const {
		iterateCells([&](size_t cellId) {
			const auto& cell = getCell(cellId);
			if (cell.getId() != cellId)
				return false;
			bool cr = cell.verify(*this, false);
			if (!cr) {
				cell.verify(*this, false);
				return false;
			}
			return true;
		});

		for (size_t vertIdx = 0; vertIdx < _verts.size(); vertIdx++) {
			const auto& vert = _verts[vertIdx];
			if (vert.getIndex() != vertIdx)
				return false;
			bool cr = vert.verify(*this, false);
			if (!cr) {
				vert.verify(*this, vertIdx);
				return false;
			}
		}

		verifyVertCount();

		map<size_t, size_t> useMap;
		for (const auto& vert : _verts) {
			size_t numCells = vert.getNumCells();
			auto iter = useMap.find(numCells);
			if (iter == useMap.end()) {
				iter = useMap.insert(make_pair(numCells, 0)).first;
			}
			iter->second++;
		}

		size_t numVertsInMap = 0;
		for (const auto& iter : useMap) {
			numVertsInMap += iter.second;
		}
		if (numVertsInMap != _verts.size()) {
			cout << "Unexpected vert usage map.\n";
			return false;
		}

		return true;
	}

	bool GridBase::verifyVertCount(size_t delta) const {
		if (_verts.size() != _vertTree.numInTree() + delta) {
			return false;
		}
		return true;
	}

	size_t GridBase::numClampedVerts() const {
		size_t num = 0;
		for (const auto& vert : _verts) {
			if (vert.getClampType() != CLAMP_NONE) {
				num++;
			}
		}
		return num;
	}

	void GridBase::dumpText(ostream& out) const {
		out << "Verts\n";

		for (const auto& vert : _verts) {
			vert.dump(out, "__");
		}

		out << "Cells\n";
		iterateCells([&](size_t cellId)->bool {
			getCell(cellId).dump(out);
			return true;
		});
	}

}
