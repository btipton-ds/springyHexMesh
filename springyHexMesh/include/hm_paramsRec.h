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

#include <tm_defines.h>
#include <iostream>

#include <hm_types.h>
#include <tm_boundingBox.h>

namespace HexahedralMesher {

	struct ParamsRec {

		double calcMaxEdgeLength() const;
		double calcMinEdgeLength() const;
		int calcMaxDivisions() const;

		void save(std::ostream& out) const;
		bool read(std::istream& in);

		double minEdgeLength = 0.01;
		double minGapSize = 0.01;
		double maxEdgeLength = 1.0;
		double sharpAngleDeg = 20.0; // Limit delta angle between perpendiculars/normals
		CBoundingBox3Dd bounds;
	};

}