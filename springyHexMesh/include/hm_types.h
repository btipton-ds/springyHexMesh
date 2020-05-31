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

#include <iostream>
#include <hm_macros.h>
#include <tm_boundingBox.h>

namespace HexahedralMesher {

	using BoundingBox = CBoundingBox3Dd;

	struct ParamsRec;

	template<int NUM_THREADS>
	class GridVertTempl;
	using GridVert = GridVertTempl<8>;

	class GridEdge;
	class GridFace;
	class GridCell;
	class Grid;

	const Vector3d vX(1, 0, 0);
	const Vector3d vY(0, 1, 0);
	const Vector3d vZ(0, 0, 1);

	enum ErrorCode {
		NO_ERR,
		UNKNOWN_ERR
	};

	enum Axis {
		X_AXIS = 0,
		Y_AXIS = 1,
		Z_AXIS = 2,
	};

	enum CellVertPos {
		LWR_FNT_LFT,
		LWR_FNT_RGT,
		LWR_BCK_LFT,
		LWR_BCK_RGT,

		UPR_FNT_LFT,
		UPR_FNT_RGT,
		UPR_BCK_LFT,
		UPR_BCK_RGT,

		CVP_UNKNOWN
	};

	enum VertEdgeDir {
		X_POS,
		Y_POS,
		Z_POS,
		X_NEG,
		Y_NEG,
		Z_NEG,
		VED_UNKNOWN
	};

	enum FaceNumber {
		BOTTOM,
		TOP,
		FRONT,
		BACK,
		LEFT,
		RIGHT,
		FN_UNKNOWN
	};

	enum ClampType {
		CLAMP_UNKNOWN = 0,
		CLAMP_NONE = 1,
		CLAMP_FIXED = 2,
		CLAMP_VERT = 4,
		CLAMP_EDGE = 8,
		CLAMP_TRI = 16,
		CLAMP_PERPENDICULAR = 32,
		CLAMP_PARALLEL = 64,
		CLAMP_CELL_EDGE_CENTER = 128,
		CLAMP_CELL_FACE_CENTER = 256,
		CLAMP_GRID_TRI_PLANE = 512,
	};

	ENUM_ITER_IMPL(Axis)
	ENUM_ITER_IMPL(CellVertPos)
	ENUM_ITER_IMPL(VertEdgeDir)
	ENUM_ITER_IMPL(FaceNumber)

	std::ostream& operator << (std::ostream& out, const ClampType& ct);
	std::istream& operator >> (std::istream& in, ClampType& ct);
}
