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

namespace HexahedralMesher {

	const CellVertPos gFaceIdxLUT[6][4] = {
		{LWR_FNT_LFT, LWR_BCK_LFT, LWR_BCK_RGT, LWR_FNT_RGT}, // bottom 0 0 0
		{UPR_FNT_LFT, UPR_FNT_RGT, UPR_BCK_RGT, UPR_BCK_LFT}, // top    0 0 1

		{LWR_FNT_LFT, LWR_FNT_RGT, UPR_FNT_RGT, UPR_FNT_LFT}, // front  0 1 0
		{LWR_BCK_LFT, UPR_BCK_LFT, UPR_BCK_RGT, LWR_BCK_RGT}, // back   

		{LWR_FNT_LFT, UPR_FNT_LFT, UPR_BCK_LFT, LWR_BCK_LFT}, // left
		{LWR_FNT_RGT, LWR_BCK_RGT, UPR_BCK_RGT, UPR_FNT_RGT}, // right
	};

	const int gVertEdgePosLUT[6][4] = {
		{ // bottom
			Y_POS, X_POS, Y_NEG, X_NEG,
		},
		{ // top
			X_POS, Y_POS, X_NEG, Y_NEG,
		},
		{ // front
			X_POS, Z_POS, X_NEG, Z_NEG,
		},
		{ // back
			Z_POS, X_POS, Z_NEG, X_NEG,
		},
		{ // left
			Z_POS, Y_POS, Z_NEG, Y_NEG,
		},
		{ // right
			Y_POS, Z_POS, Y_NEG, Z_NEG,
		},
	};

	const CellVertPos gPosEdgePosLT[8][6] = {
		{ // LWR_FNT_LFT
			LWR_FNT_RGT, // X_POS,
			LWR_BCK_LFT, // Y_POS,
			UPR_FNT_LFT, // Z_POS,
			CVP_UNKNOWN, // X_NEG,
			CVP_UNKNOWN, // Y_NEG,
			CVP_UNKNOWN, // Z_NEG,
		},
		{ // LWR_FNT_RGT
			CVP_UNKNOWN, // X_POS,
			LWR_BCK_RGT, // Y_POS,
			UPR_FNT_RGT, // Z_POS,
			LWR_FNT_LFT, // X_NEG,
			CVP_UNKNOWN, // Y_NEG,
			CVP_UNKNOWN, // Z_NEG,
		},
		{ // LWR_BCK_LFT
			LWR_BCK_RGT, // X_POS,
			CVP_UNKNOWN, // Y_POS,
			UPR_BCK_LFT, // Z_POS,
			CVP_UNKNOWN, // X_NEG,
			LWR_FNT_LFT, // Y_NEG,
			CVP_UNKNOWN, // Z_NEG,
		},
		{ // LWR_BCK_RGT
			CVP_UNKNOWN, // X_POS,
			CVP_UNKNOWN, // Y_POS,
			UPR_BCK_RGT, // Z_POS,
			LWR_BCK_LFT, // X_NEG,
			LWR_FNT_RGT, // Y_NEG,
			CVP_UNKNOWN, // Z_NEG,
		},

		{ // UPR_FNT_LFT
			UPR_FNT_RGT, // X_POS,
			UPR_BCK_LFT, // Y_POS,
			CVP_UNKNOWN, // Z_POS,
			CVP_UNKNOWN, // X_NEG,
			CVP_UNKNOWN, // Y_NEG,
			LWR_FNT_LFT, // Z_NEG,
		},
		{ // UPR_FNT_RGT
			CVP_UNKNOWN, // X_POS,
			UPR_BCK_RGT, // Y_POS,
			CVP_UNKNOWN, // Z_POS,
			UPR_FNT_LFT, // X_NEG,
			CVP_UNKNOWN, // Y_NEG,
			LWR_FNT_RGT, // Z_NEG,
		},
		{ // UPR_BCK_LFT
			UPR_BCK_RGT, // X_POS,
			CVP_UNKNOWN, // Y_POS,
			CVP_UNKNOWN, // Z_POS,
			CVP_UNKNOWN, // X_NEG,
			UPR_FNT_LFT, // Y_NEG,
			LWR_BCK_LFT, // Z_NEG,
		},
		{ // UPR_BCK_RGT
			CVP_UNKNOWN, // X_POS,
			CVP_UNKNOWN, // Y_POS,
			CVP_UNKNOWN, // Z_POS,
			UPR_BCK_LFT, // X_NEG,
			UPR_FNT_RGT, // Y_NEG,
			LWR_BCK_RGT, // Z_NEG,
		},
	};

	const CellVertPos gEdgeVerts[12][2] = {
		{LWR_FNT_LFT, LWR_FNT_RGT}, // Front face
		{LWR_FNT_RGT, UPR_FNT_RGT},
		{UPR_FNT_RGT, UPR_FNT_LFT},
		{UPR_FNT_LFT, LWR_FNT_LFT},

		{LWR_BCK_LFT, LWR_BCK_RGT}, // Back face
		{LWR_BCK_RGT, UPR_BCK_RGT},
		{UPR_BCK_RGT, UPR_BCK_LFT},
		{UPR_BCK_LFT, LWR_BCK_LFT},

		{LWR_FNT_LFT, LWR_BCK_LFT}, // Front to back
		{LWR_FNT_RGT, LWR_BCK_RGT},
		{UPR_FNT_RGT, UPR_BCK_RGT},
		{UPR_FNT_LFT, UPR_BCK_LFT},
	};

	const FaceNumber gPosFaceNumberLUT[8][3] = {
		{ // LWR_FNT_LFT
			BOTTOM, LEFT, FRONT,
		},
		{ // LWR_FNT_RGT
			BOTTOM, RIGHT, FRONT,
		},
		{ // LWR_BCK_LFT
			BOTTOM, LEFT, BACK,
		},
		{ // LWR_BCK_RGT
			BOTTOM, RIGHT, BACK,
		},

		{ // LWR_FNT_LFT
			TOP, LEFT, FRONT,
		},
		{ // LWR_FNT_RGT
			TOP, RIGHT, FRONT,
		},
		{ // LWR_BCK_LFT
			TOP, LEFT, BACK,
		},
		{ // LWR_BCK_RGT
			TOP, RIGHT, BACK,
		},
	};
		
	const CellVertPos gOppCornerLUT[] = {
		UPR_BCK_RGT,
		UPR_BCK_LFT,
		UPR_FNT_RGT,
		UPR_FNT_LFT,
		LWR_BCK_RGT,
		LWR_BCK_LFT,
		LWR_FNT_RGT,
		LWR_FNT_LFT,
	};

	const FaceNumber gOppFaceLUT[] = {
		TOP,
		BOTTOM,
		BACK,
		FRONT,
		RIGHT,
		LEFT,
	};

}
