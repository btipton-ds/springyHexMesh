#pragma once

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
#include <triMesh.h>
#include <hm_paramsRec.h>

namespace HexahedralMesher {

	class GridEnergy {
	public:
		struct Params {
			inline Params()
				: _kCompress(0.001)
				, _pCompress(2.0)
				, _kBend(1.0)
				, _pBend(1.0)
			{}
			inline Params(const Params& src) = default;

			double _kCompress, _pCompress, _kBend, _pBend;
		};

		GridEnergy(const Grid& grid, const Params& params = Params());

		double calcTotalEnergy(const GridCell& cell) const;
		double calcCompressionEnergy(const GridCell& cell) const;
		double calcBendEnergy(const GridCell& cell) const;

		double calcTotalEnergy(const GridVert& vert) const;
		double calcCompressionEnergy(const GridVert& vert) const;
		double calcBendEnergy(const GridVert& vert) const;

	private:
		double calcTotalEnergy(double orthoEnergy, double volumeEnergy) const;

		const Params _params;
		const Grid& _grid;
	};
}