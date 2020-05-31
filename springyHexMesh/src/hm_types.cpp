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
#include <hm_types.h>

namespace HexahedralMesher {


	std::ostream& operator << (std::ostream& out, const ClampType& ct) {
		switch (ct) {
		case CLAMP_NONE:
			out << "CLAMP_NONE";
			break;
		case CLAMP_FIXED:
			out << "CLAMP_FIXED";
			break;
		case CLAMP_VERT:
			out << "CLAMP_VERT";
			break;
		case CLAMP_EDGE:
			out << "CLAMP_EDGE";
			break;
		case CLAMP_TRI:
			out << "CLAMP_TRI";
			break;
		case CLAMP_PERPENDICULAR:
			out << "CLAMP_PERPENDICULAR";
			break;
		case CLAMP_PARALLEL: out <<
			"CLAMP_PARALLEL";
			break;
		case CLAMP_CELL_EDGE_CENTER:
			out << "CLAMP_CELL_EDGE_CENTER";
			break;
		case CLAMP_CELL_FACE_CENTER:
			out << "CLAMP_CELL_FACE_CENTER";
			break;
		case CLAMP_GRID_TRI_PLANE:
			out << "CLAMP_GRID_TRI_PLANE";
			break;
		default:
			break;
		}
		return out;
	}

	std::istream& operator >> (std::istream& in, ClampType& ct) {
		std::string str;
		in >> str;

		if (str == "CLAMP_UNKNOWN") throw "should never save an unknown clamp";
		else if (str == "CLAMP_NONE") ct = CLAMP_NONE;
		else if (str == "CLAMP_FIXED") ct = CLAMP_FIXED;
		else if (str == "CLAMP_VERT") ct = CLAMP_VERT;
		else if (str == "CLAMP_EDGE") ct = CLAMP_EDGE;
		else if (str == "CLAMP_TRI") ct = CLAMP_TRI;
		else if (str == "CLAMP_PERPENDICULAR") ct = CLAMP_PERPENDICULAR;
		else if (str == "CLAMP_PARALLEL") ct = CLAMP_PARALLEL;
		else if (str == "CLAMP_CELL_EDGE_CENTER") ct = CLAMP_CELL_EDGE_CENTER;
		else if (str == "CLAMP_CELL_FACE_CENTER") ct = CLAMP_CELL_FACE_CENTER;
		else if (str == "CLAMP_GRID_TRI_PLANE") ct = CLAMP_GRID_TRI_PLANE;
		else
			throw "Unexpected clamptype on read";

		return in;
	}


}