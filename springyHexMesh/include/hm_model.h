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

#include <vector>
#include <memory>

#include <tm_spatialSearch.h>
#include <triMesh.h>
#include <tm_polyLine.h>

namespace HexahedralMesher {

	class CModel : public TriMesh::CMesh {
	public:
		using SearchTree = TriMesh::CMesh::SearchTree;

		CModel(double sharpAngleDeg);

		void init();

		void save(std::ostream& out) const;
		bool read(std::istream& in);
		inline bool polylineExists(size_t idx) const {
			return idx < _polyLines.size();
		}

		const double _sinSharpAngle;
		SearchTree _sharpEdgeTree;
		std::set<size_t> _cusps;
		std::vector<TriMesh::CPolyLine> _polyLines;

	private:
		void findSharpEdges(std::vector<size_t>& sharps);
		void createPolylines(std::vector<size_t>& sharps);
		void createCuspsAndSplitPolylines();
	};

}