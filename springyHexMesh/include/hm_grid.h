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

#include <hm_gridBase.h>

namespace HexahedralMesher {

	class CMesher;

	class Grid : public GridBase {
		friend class GridCell;
	public:

		Grid(CMesher& mesher);

		void init(const BoundingBox& bbox);
		bool verify() const;

		const CMesher& getMesher() const;
		CMesher& getMesher();

		const ParamsRec& getParams() const;
		size_t getVertsFaces(size_t vertIdx, bool includeOpposedPairs, std::vector<GridFace>& faceRefs) const;
		double calcCellEnergy(size_t cellId) const;
		double calcVertexEnergy(size_t vertIdx) const;
		double calcVertexEnergyAtPos(size_t vertIdx, const Vector3d& atPos);
		double calcVertexOrthoEnergy(size_t vertIdx) const;
		double calcVertexOrthoEnergyAtPos(size_t vertIdx, const Vector3d& atPos);
		Vector3d calcTriangleCentroid(size_t vertIdx[3]) const;

		double minimizeVertexEnergy(std::ostream& logOut, size_t vertIdx, int clampMask);

		double clampVertex(size_t vertIdx);
		double clampVertexToTriPlane(size_t vertIdx);
		double clampVertexToCellEdgeCenter(size_t vertIdx);
		double clampVertexToCellFaceCenter(size_t vertIdx);

	private:
		struct ScopedSetStash {
			ScopedSetStash(Grid& grid, size_t vertIdx);
			~ScopedSetStash();

			Grid& _grid;
			size_t _vertIdx;
			Vector3d _pt;
			TopolRef _clamp;
		};

		template<typename GRAD_FUNC, typename LOG_FUNC>
		double minimizeVertexEnergy(size_t vertIdx, LOG_FUNC logFunc, GRAD_FUNC gradFunc);

		double calcEnergyGradientFree(size_t vertIdx, double dt, Vector3d& gradient);
		double calcEnergyGradientPerpendicular(size_t vertIdx, const Vector3d& normal, double dt, Vector3d& gradient);
		double calcEnergyGradientTriPlane(size_t vertIdx, double dt, Vector3d& gradient);
		double calcEnergyGradientEdge(size_t vertIdx, double dt, Vector3d& gradient);
		int chooseBestGradient(size_t vertIdx, double dt, const Vector3d gradients[], int numGradients);

		double calcMoveDist(size_t vertIdx, double dt, const Vector3d& gradient);
		void fixGradientDirection(size_t vertIdx, double dt, Vector3d& gradient);


		CMesher* _mesher;
	};


	inline const CMesher& Grid::getMesher() const {
		return *_mesher;
	}

	inline CMesher& Grid::getMesher() {
		return *_mesher;
	}

}
