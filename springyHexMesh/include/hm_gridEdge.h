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
#include <hm_gridFace.h>

namespace HexahedralMesher {
	class GridBase;
	class TopolRef;

	class GridEdge {
	public:
		GridEdge(size_t vertIdx0 = stm1, size_t vertIdx1 = stm1);
		bool operator < (const GridEdge& rhs) const;
		bool operator == (const GridEdge& rhs) const;
		size_t getVert(int idx) const;

		LineSegment calcSeg(const GridBase& grid) const;
		Vector3d calcDir(const GridBase& grid) const;
		double calcLength(const GridBase& grid) const;
		Vector3d calcCenter(const GridBase& grid) const;
		bool isBoundary(const Grid& grid, TopolRef& clamp) const;
		double distToPoint(const Grid& grid, const Vector3d& pt, double& t) const;

		void dump(std::ostream& out, const GridBase& grid, std::string pad) const;

	private:
		size_t _vertIndices[2];
	};

	inline size_t GridEdge::getVert(int idx) const {
		return _vertIndices[idx];
	}

}