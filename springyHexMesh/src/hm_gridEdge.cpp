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
#include <hm_gridEdge.h>
#include <hm_gridVert.h>
#include <hm_grid.h>
#include <hm_topolRef.h>

namespace HexahedralMesher {

	using namespace std;

	GridEdge::GridEdge(size_t vertIdx0, size_t vertIdx1) {
		if (vertIdx0 < vertIdx1) {
			_vertIndices[0] = vertIdx0;
			_vertIndices[1] = vertIdx1;
		}
		else {
			_vertIndices[0] = vertIdx1;
			_vertIndices[1] = vertIdx0;
		}
	}

	bool GridEdge::operator < (const GridEdge& rhs) const {
		for (int i = 0; i < 2; i++) {
			if (_vertIndices[i] < rhs._vertIndices[i])
				return true;
			else if (_vertIndices[i] > rhs._vertIndices[i])
				return false;
		}
		return false;
	}

	bool GridEdge::operator == (const GridEdge& rhs) const {
		return 
			_vertIndices[0] == rhs._vertIndices[0] && 
			_vertIndices[1] == rhs._vertIndices[1];
	}

	LineSegment GridEdge::calcSeg(const GridBase& grid) const {
		return LineSegment(grid.getVert(_vertIndices[0]).getPt(), grid.getVert(_vertIndices[1]).getPt());
	}

	Vector3d GridEdge::calcDir(const GridBase& grid) const {
		LineSegment seg = calcSeg(grid);
		return seg.calcDir();
	}

	double GridEdge::calcLength(const GridBase& grid) const {
		LineSegment seg = calcSeg(grid);
		return seg.calLength();
	}

	Vector3d GridEdge::calcCenter(const GridBase& grid) const {
		const Vector3d& pt0 = grid.getVert(_vertIndices[0]).getPt();
		const Vector3d& pt1 = grid.getVert(_vertIndices[1]).getPt();
		return (pt0 + pt1) * 0.5;
	}

	bool GridEdge::isBoundary(const Grid& grid, TopolRef& clamp) const {
		const auto& vert0 = grid.getVert(_vertIndices[0]);
		const auto& vert1 = grid.getVert(_vertIndices[1]);

		const auto& clamp0 = grid.getVert(_vertIndices[0]).getClamp();
		const auto& clamp1 = grid.getVert(_vertIndices[1]).getClamp();

		if (clamp0.matches(CLAMP_PARALLEL | CLAMP_FIXED) && clamp1.matches(CLAMP_PARALLEL | CLAMP_FIXED)) {
			Vector3d edgeDir = calcDir(grid);
			if (clamp0.getClampType() == CLAMP_PARALLEL) {
				if ((1 - fabs(clamp0.getVector().dot(edgeDir) < 1.0e-6))) {
					clamp = clamp0;
					return true;
				}
			}
			else if (clamp1.getClampType() == CLAMP_PARALLEL) {
				if ((1 - fabs(clamp1.getVector().dot(edgeDir) < 1.0e-6))) {
					clamp = clamp1;
					return true;
				}
			}
		}
		return false;
	}

	double GridEdge::distToPoint(const Grid& grid, const Vector3d& pt, double& t) const {
		LineSegment seg = calcSeg(grid);
		return seg.distanceToPoint(pt, t);
	}

	void GridEdge::dump(std::ostream& out, const GridBase& grid, std::string pad) const {
		Vector3d pts[] = {
			grid.getVert(_vertIndices[0]).getPt(),
			grid.getVert(_vertIndices[1]).getPt()
		};
		Vector3d v = (pts[1] - pts[0]).normalized();

		out << pad << "edge\n";
		pad += pad;
		out << pad << "index(" << _vertIndices[0] << ", " << _vertIndices[1] << ")\n";
		out << pad << "verts(";

		for (int j = 0; j < 2; j++) {
			for (int i = 0; i < 3; i++) {
				out << pts[j][i];
				if (i < 2)
					out << ", ";

			}
			out << ")";
			if (j == 0) 
				out << ", (";
		}
		out << ")\n";

		out << pad << "dir(";

		for (int i = 0; i < 3; i++) {
			out << v[i];
			if (i < 2)
				out << ", ";

		}
		out << ")\n";
	}
}
