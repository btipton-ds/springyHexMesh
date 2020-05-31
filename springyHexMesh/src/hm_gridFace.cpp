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

#include <hm_gridFace.h>

#include <set>

#include <hm_gridVert.h>
#include <hm_grid.h>

namespace HexahedralMesher {

	using namespace std;

	Vector3d GridFace::calcNormal(const Grid& grid) const {
		const Vector3d* tris[2][3];
		const auto& cell = grid.getCell(_cellIdx);
		cell.getFaceTriPoints(grid, _face, tris);
		Vector3d result = triangleNormal(tris[0]) + triangleNormal(tris[1]);
		result.normalize();
		return result;
	}

	Vector3d GridFace::calcCentroid(const Grid& grid) const {
		const Vector3d* pts[4];
		const auto& cell = grid.getCell(_cellIdx);
		cell.getFacePoints(grid, _face, pts);
		Vector3d result = ngonCentroid(4, pts);
		return result;
	}

	void GridFace::getPoints(const Grid& grid, const Vector3d* points[4]) const {
		const auto& cell = grid.getCell(_cellIdx);
		cell.getFacePoints(grid, _face, points);
	}

	void GridFace::getVertIndices(const Grid& grid, size_t verts[4]) const {
		const auto& cell = grid.getCell(getCellIdx());
		cell.getFaceIndices(getFaceNumber(), verts);
	}

	void GridFace::getTriIndices(const Grid& grid, size_t tri[2][3]) const {
		const auto& cell = grid.getCell(getCellIdx());
		cell.getFaceTriIndices(getFaceNumber(), tri);
	}

	void GridFace::getTriVertPtrs(const Grid& grid, const Vector3d* tri[2][3]) const {
		const auto& cell = grid.getCell(getCellIdx());
		cell.getFaceTriPoints(grid, _face, tri);
	}

	bool GridFace::sameVerts(size_t verts0[4], size_t verts1[4]) {
		for (int i = 0; i < 4; i++) {
			bool found = false;
			for (int j = 0; j < 4; j++) {
				if (verts0[i] == verts1[j]) {
					found = true;
					break;
				}
			}
			if (!found)
				return false;
		}
		return true;
	}

	bool GridFace::sameVerts(const Grid& grid, const GridFace& other) const {
		const GridCell& cell0 = grid.getCell(getCellIdx());
		const GridCell& cell1 = grid.getCell(other.getCellIdx());

		size_t verts0[4], verts1[4];
		cell0.getFaceIndices(getFaceNumber(), verts0);
		cell1.getFaceIndices(other.getFaceNumber(), verts1);

		return sameVerts(verts0, verts1);
	}


	SearchableFace::SearchableFace(const GridFace& face, const size_t indices[4])
		: GridFace(face)
	{
		set<size_t> verts;
		for (int i = 0; i < 4; i++)
			verts.insert(indices[i]);

		int count = 0;
		for (size_t vertIdx : verts)
			_sortedIndices[count++] = vertIdx;
	}

	bool SearchableFace::operator < (const SearchableFace& rhs) const {
		for (int i = 0; i < 4; i++) {
			if (_sortedIndices[i] < rhs._sortedIndices[i])
				return true;
			else if (_sortedIndices[i] > rhs._sortedIndices[i])
				return false;
		}
		return false;

	}


}