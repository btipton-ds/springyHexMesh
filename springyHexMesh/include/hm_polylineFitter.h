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

#include <vector>
#include <map>

#include <hm_types.h>
#include <hm_forwardDeclarations.h>
#include <hm_model.h>
#include <triMesh.h>

namespace HexahedralMesher {

	class CPolylineFitter {
	public:
		CPolylineFitter(CMesher& mesher, size_t meshIdx, size_t polylineNumber);

		size_t doFit(std::set<size_t>& cellsToSplit);

	private:
		struct FaceHit;

		size_t findStartingCornerIndex() const;
		bool findCellFaceHits(size_t cornerIdx, std::map<size_t, std::vector<FaceHit>>& hits,
			std::set<size_t>& cellsToSplit) const;
		bool fitCells(const std::map<size_t, std::vector<FaceHit>>& hits, size_t& cornerIdx, size_t& plIdx, 
			std::set<size_t>& cellsToSplit);
		size_t findPierces(size_t cellIdx, CellVertPos corner, std::vector<FaceHit>& hits) const;
		size_t polylineIntersectsTri(size_t startIdx, const Vector3d* tri[3], RayHit& hit) const;
		CellVertPos findClosestUnclampedCorner(size_t cellIdx, CellVertPos ignorePos, const FaceHit& faceHit) const;
		void putUnclampedCornerOnPolyline(size_t cellIdx, CellVertPos corner, const FaceHit& faceHit);
		void addPolylineEndToClamped();
		void getClampedCells(std::set<size_t>& cellsToSplit);

		CMesher& _mesher;
		size_t _meshIdx;
		size_t _polylineNum;
		Grid& _grid;
		const ParamsRec& _params;
		const CModelPtr& _modelPtr;
		const TriMesh::CPolyLine& _polyline;

		std::vector<size_t> _clampedVertIndices;
	};

}
