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

#include <hm_topolRef.h>

#include <iomanip>

#include <hm_grid.h>
#include <hm_model.h>
#include <meshProcessor.h>

namespace HexahedralMesher {

	using namespace std;

	void TopolRef::save(ostream& out) const {
		out << "CT: " << _clampType << "\n";

		switch (_clampType) {
		case CLAMP_VERT:
			// TODO get rid of the vertex index, we don't use it.
			out << "  RI: " << _indices[0] << " " << _indices[1] << "\n";
			break;

		case CLAMP_GRID_TRI_PLANE:
		case CLAMP_EDGE:
		case CLAMP_TRI:
			out << "  RI: " << _indices[0] << " " << _indices[1] << " " << _indices[2] << "\n";
			break;

		case CLAMP_PERPENDICULAR:
		case CLAMP_PARALLEL:
			out << fixed << setprecision(filePrecision) << "  V: " << _v[0] << " " << _v[1] << " " << _v[2] << "\n";
			break;

		case CLAMP_CELL_EDGE_CENTER:
		case CLAMP_CELL_FACE_CENTER:
			out << "  RI: " << _indices[0] << " " << _indices[1] << "\n";
			break;
		default:
			break;
		}

	}

	bool TopolRef::verify(const GridBase& gridBase) const {
		const Grid& grid = static_cast<const Grid&> (gridBase);
		switch (_clampType) {
		case CLAMP_VERT:
			if (!grid.getMesher().modelExists(getMeshIdx()))
				return false;
			// The vertex index isn't actually used, it's treated as fixed.
			break;

		case CLAMP_GRID_TRI_PLANE:
			for (int i = 0; i < 3; i++)
			if (!grid.vertExists(_indices[i]))
				return false;
			break;

		case CLAMP_EDGE: {
			const auto& mesher = grid.getMesher();
			if (!mesher.modelExists(getMeshIdx()))
				return false;
			const auto& modelPtr = mesher.getModelPtr(getMeshIdx());
			if (!modelPtr->polylineExists(getPolylineNumber()))
				return false;
			const auto& pl = modelPtr->_polyLines[getPolylineNumber()];
			return pl.isValidIndex(getPolylineIndex());
		}
		default:
			break;
		}
		return true;
	}

}