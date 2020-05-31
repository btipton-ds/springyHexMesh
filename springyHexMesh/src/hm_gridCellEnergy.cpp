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

#include <hm_gridCellEnergy.h>

#include <vector>
#include <fstream>
#include <iomanip>

#include <hm_tables.h>
#include <hm_paramsRec.h>
#include <hm_gridVert.h>
#include <hm_gridCell.h>
#include <hm_grid.h>

namespace HexahedralMesher {

	using namespace std;

	constexpr double kScale = 3600; // Constant determined to put the energy value at 1 for 5% displactement of a point

	GridEnergy::GridEnergy(const Grid& grid, const Params& params)
		: _grid(grid)
		, _params(params)
	{}

	double GridEnergy::calcTotalEnergy(double orthoEnergy, double volumeEnergy) const {
		double result = 0;
		result += _params._kBend * pow(orthoEnergy, _params._pBend);
		result += _params._kCompress * pow(volumeEnergy, _params._pCompress);
#if 0
		static double maxEV = 0;
		static double maxEO = 0;
		static double maxResult = 0;

		if (volumeEnergy > maxEV || orthoEnergy > maxEO || result > maxResult) {
			if (volumeEnergy > maxEV)
				maxEV = volumeEnergy;
			if (orthoEnergy > maxEO)
				maxEO = orthoEnergy;
			if (result > maxResult)
				maxResult = result;
			cout << "maxResult: " << maxResult << ", maxEV: " << maxEV << ", maxEO: " << maxEO << "\n";
		}
#endif
		return result;
	}

	double GridEnergy::calcTotalEnergy(const GridCell& cell) const {
		double orthoEnergy = 0, volumeEnergy = 0;
		if (_params._kBend > 0)
			orthoEnergy = calcBendEnergy(cell);
		if (_params._kCompress > 0)
			volumeEnergy = calcCompressionEnergy(cell);

		return calcTotalEnergy(orthoEnergy, volumeEnergy);
	}

	double GridEnergy::calcCompressionEnergy(const GridCell& cell) const {
		const double k = 10;
		const double minRatio = 1;
		double totalEnergy = 0;
		for (int edgeNum = 0; edgeNum < 12; edgeNum++) {
			double minLen = minRatio * cell.getRestEdgeLength(edgeNum);
			GridEdge edge = cell.getEdge(edgeNum);
			double len = edge.calcLength(_grid);
			double deltaL = len - minLen;
			double e = k * deltaL * deltaL;
			checkNAN(e);
			totalEnergy += e;
		}

		return totalEnergy;
	}

	struct SignedVector {
		double sign;
		CellVertPos pos;
	};

	const SignedVector gOrientedEdgePosLT[8][3] = {
	{ // LWR_FNT_LFT
		{1, LWR_FNT_RGT},
		{1, LWR_BCK_LFT},
		{1, UPR_FNT_LFT},
	},
	{ // LWR_FNT_RGT
		{ 1, UPR_FNT_RGT},
		{ 1, LWR_BCK_RGT},
		{-1, LWR_FNT_LFT},
	},
	{ // LWR_BCK_LFT
		{-1, LWR_FNT_LFT},
		{ 1, LWR_BCK_RGT},
		{ 1, UPR_BCK_LFT},
	},
	{ // LWR_BCK_RGT
		{-1, LWR_BCK_LFT},
		{-1, LWR_FNT_RGT},
		{ 1, UPR_BCK_RGT},
	},

	{ // UPR_FNT_LFT
		{ 1, UPR_FNT_RGT},
		{-1, LWR_FNT_LFT},
		{ 1, UPR_BCK_LFT},
	},
	{ // UPR_FNT_RGT
		{ 1, UPR_BCK_RGT},
		{-1, LWR_FNT_RGT},
		{-1, UPR_FNT_LFT},
	},
	{ // UPR_BCK_LFT
		{-1, UPR_FNT_LFT},
		{-1, LWR_BCK_LFT},
		{ 1, UPR_BCK_RGT},
	},
	{ // UPR_BCK_RGT
		{-1, UPR_FNT_RGT},
		{-1, UPR_BCK_LFT},
		{-1, LWR_BCK_RGT},
	},
	};

	double GridEnergy::calcBendEnergy(const GridCell& cell) const {
		double totalEnergy = 0;
		const double k = 1000;

		for (CellVertPos pos0 = LWR_FNT_LFT; pos0 < CVP_UNKNOWN; pos0++) {
			size_t vertIdx = cell.getVertIdx(pos0);
			Vector3d edgeDirs[3];
			const SignedVector* adjEdgePos = gOrientedEdgePosLT[pos0];
			for (int i = 0; i < 3; i++) {
				const auto& adj = adjEdgePos[i];
				GridEdge edge(vertIdx, cell.getVertIdx(adj.pos));
				if (edge.getVert(0) == vertIdx)
					edgeDirs[i] = edge.calcDir(_grid);
				else
					edgeDirs[i] = -edge.calcDir(_grid);
			}
			for (int i = 0; i < 3; i++) {
				const Vector3d& vI = edgeDirs[i];
				const Vector3d& vJ = edgeDirs[(i + 1) % 3];
				const Vector3d& vK = edgeDirs[(i + 2) % 3];

				Vector3d normal = vI.cross(vJ);


				double cos = normal.dot(vK);
				checkNAN(cos);
				double sin = normal.cross(vK).norm();
				checkNAN(sin);
				double theta = atan2(sin, cos);
				double thetaDeg = theta / EIGEN_PI * 180;
				theta = theta / EIGEN_PI;
				checkNAN(theta);
				double e = k * theta * theta;
				if (e > 0) {
					int dbgBreak = 1;
				}
				totalEnergy += e;
				checkNAN(totalEnergy);
			}
		}

		if (totalEnergy > 1.0e5) {
			throw "Energy out of bounds";
		}
		return totalEnergy;
	}

	double GridEnergy::calcTotalEnergy(const GridVert& vert) const {
		double result = 0;

		const auto& cellIndices = vert.getCellIndices();
		for (size_t cellIdx : cellIndices) {
			const auto& cell = _grid.getCell(cellIdx);
			result += calcTotalEnergy(cell);
		}

		return result;
	}

	double GridEnergy::calcCompressionEnergy(const GridVert& vert) const {
		double result = 0;

		const auto& cellIndices = vert.getCellIndices();
		for (size_t cellIdx : cellIndices) {
			const auto& cell = _grid.getCell(cellIdx);
			result += calcCompressionEnergy(cell);
		}

		return result;
	}

	double GridEnergy::calcBendEnergy(const GridVert& vert) const {
		double result = 0;

		const auto& cellIndices = vert.getCellIndices();
		for (size_t cellIdx : cellIndices) {
			const auto& cell = _grid.getCell(cellIdx);
			result += calcBendEnergy(cell);
		}

		return result;

	}

}

