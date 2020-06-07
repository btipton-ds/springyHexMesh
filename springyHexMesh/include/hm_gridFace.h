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

#include <hm_types.h>

#include <tm_math.h>

namespace HexahedralMesher {

	class GridBase;
	class Grid;

	class GridFace {
	public:
		static bool sameVerts(size_t verts0[4], size_t verts1[4]);

		GridFace(size_t cellIdx = stm1, FaceNumber face = FN_UNKNOWN);
		size_t getCellIdx() const;
		FaceNumber getFaceNumber() const;

		Vector3d calcNormal(const Grid& grid) const;
		Vector3d calcCentroid(const Grid& grid) const;
		void getPoints(const Grid& grid, const Vector3d* points[4]) const;
		void getVertIndices(const Grid& grid, size_t verts[4]) const;
		void getTriIndices(const Grid& grid, size_t tri[2][3]) const;
		void getTriVertPtrs(const Grid& grid, const Vector3d* tri[2][3]) const;
		bool sameVerts(const Grid& grid, const GridFace& other) const;

		bool operator < (const GridFace& rhs) const;
		bool operator == (const GridFace& rhs) const;
		bool operator != (const GridFace& rhs) const;

	private:
		size_t _cellIdx = stm1;
		FaceNumber _face = FN_UNKNOWN;
	};

	inline GridFace::GridFace(size_t cellIdx, FaceNumber face)
		: _cellIdx(cellIdx)
		, _face(face)
	{}

	inline size_t GridFace::getCellIdx() const {
		return _cellIdx;
	}

	inline FaceNumber GridFace::getFaceNumber() const {
		return _face;
	}

	inline bool GridFace::operator < (const GridFace& rhs) const {
		if (_cellIdx == rhs._cellIdx)
			return _face < rhs._face;
		else 
			return _cellIdx < rhs._cellIdx;
	}

	inline bool GridFace::operator == (const GridFace& rhs) const {
		return _cellIdx == rhs._cellIdx && _face == rhs._face;
	}

	inline bool GridFace::operator != (const GridFace& rhs) const {
		return !operator == (rhs);
	}

	class SearchableFace : public GridFace {
	public:
		SearchableFace(const GridFace& face, const size_t indices[4]);
		bool operator < (const SearchableFace& rhs) const;
	private:
		size_t _sortedIndices[4];
	};



}