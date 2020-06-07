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

#include <iomanip>
#include <iostream>
#include <fstream>

#include <hm_gridVert.h>
#include <hm_gridCell.h>
#include <hm_gridBase.h>
#include <hm_grid.h>

namespace HexahedralMesher {
	using namespace std;

	bool TopolRef::readVersion1(std::istream& in) {
		string str;
		in >> str >> _clampType;
		if (str != "CT:") return false;

		switch (_clampType) {
		case CLAMP_VERT:
			in >> str >> _indices[0] >> _indices[1];
			if (str != "RI:") return false;
			break;

		case CLAMP_GRID_TRI_PLANE:
		case CLAMP_EDGE:
		case CLAMP_TRI:
			in >> str >> _indices[0] >> _indices[1] >> _indices[2];
			if (str != "RI:") return false;
			break;

		case CLAMP_CELL_EDGE_CENTER:
		case CLAMP_CELL_FACE_CENTER:
			in >> str >> _indices[0] >> _indices[1];
			if (str != "RI:") return false;
			break;

		case CLAMP_PERPENDICULAR:
		case CLAMP_PARALLEL:
			in >> str >> _v[0] >> _v[1] >> _v[2];
			if (str != "V:") return false;
			break;

		default:
			break;
		}

		return true;
	}


	bool GridCell::readVersion1(std::istream& in) {
		string str;
		in >> str >> _id;
		if (str != "ID:") return false;

		in >> str;
		if (str != "REL:") return false;
		for (int i = 0; i < 12; i++)
			in >> _restEdgeLen[i];

		in >> str;
		if (str != "VI:") return false;

		for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
			size_t val;
			in >> val;
			_vertIndices[p] = val;
		}

		return true;
	}

	bool GridBase::readVersion1(istream& in) {
		string str;
		size_t numVerts;
		in >> str >> numVerts;
		if (str != "Verts")
			return false;
		_verts.resize(numVerts);

		BoundingBox bbox(_vertTree.getBounds());
		for (size_t i = 0; i < numVerts; i++) {
			if (!_verts[i].readVersion1(in))
				return false;
			BoundingBox bb(_verts[i].getPt());
			bb.grow(SAME_DIST_TOL);
			bbox.merge(bb);
		}

		size_t cellIndexMapSize;
		in >> str >> cellIndexMapSize;
		if (str != "CellIndexMap") return false;

		_cellIndexMap.resize(cellIndexMapSize, stm1);

		size_t idx, cellId;
		for (size_t i = 0; i < cellIndexMapSize; i++) {
			in >> idx >> cellId;
			_cellIndexMap[idx] = cellId;
			i = idx;
		}

		size_t numCells;
		in >> str >> numCells;
		if (str != "Cells") return false;

		_cellStorage.resize(numCells);
		for (size_t i = 0; i < numCells; i++) {
			if (!_cellStorage[i].readVersion1(in))
				return false;
		}

		_vertTree.reset(bbox);
		rebuildVertTree();

		if (!verify()) {
			cout << "Grid failed verification.\n";
		}

		return true;
	}

}
