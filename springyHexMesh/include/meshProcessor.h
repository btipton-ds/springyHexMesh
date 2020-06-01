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

/*

Basic triangle mesh. Suitable for storing STL, obj etc.

This is the primary input to the hexahedron mesher

This code is the sole property of Robert R Tipton and Dark Sky Innovative Solutions.
All rights reserved

*/

#include <tm_defines.h>

#include <string>
#include <map>
#include <set>
#include <memory>
#include <algorithm>

#include <hm_model.h>

#include <hm_types.h>
#include <hm_forwardDeclarations.h>
#include <hm_paramsRec.h>
#include <hm_gridVert.h>
#include <hm_gridFace.h>
#include <hm_gridCell.h>
#include <hm_grid.h>
#include <hm_dumpObj.h>

using namespace std;

namespace HexahedralMesher {
	
	class CMesher;
	using CMesherPtr = std::shared_ptr<CMesher>;

	class CMesher {
	public:
		struct PolylineRec;
		struct CellPolylineRec;
		struct CellMeshRec;

		class Reporter;
		using ReporterPtr = std::shared_ptr<Reporter>;

		class Reporter {
		public:
			virtual void report(const CMesher& mesher, const std::string& key) const;
			virtual void reportModelAdded(const CMesher& mesher, const CModelPtr& model);
		};

		CMesher(const ParamsRec& params, const ReporterPtr& reporter);

		void reset();
		bool addFile(const std::string& path, const std::string& filename);
		void addModel(const std::shared_ptr<CModel>& modelPtr);
		bool modelExists(size_t modelIdx) const;


		void save(const std::string& path) const;
		void save(std::ostream& out) const;
		bool read(const std::string& path);
		bool read(std::istream& in);

		void splitCellsAroundPolylines();
		void snapToCusps();

		inline const Grid& getGrid() const {
			return _grid;
		}

		inline Grid& getGrid() {
			return _grid;
		}

		inline const ParamsRec& getParams() const {
			return _params;
		}

		size_t getNumModels() const {
			return _modelPtrs.size();
		}

		const CModelPtr& getModelPtr(size_t modelIdx) const {
			return _modelPtrs[modelIdx];
		}

		CModelPtr& getModelPtr(size_t modelIdx) {
			return _modelPtrs[modelIdx];
		}

		ErrorCode run();

	private:
		double findMinimumGap() const;
		size_t findVertFaces(size_t vertIdx, std::set<GridFace>& faceSet) const;

		void divideMesh(int numDivisions);
		void putCornersOnSharpEdges();
		void splitCells(int numSplits);
		template<class FUNC>
		void iterateCellMeshRecMap(map<size_t, CellMeshRec>& cellSegs, FUNC func);

		void intersectMesh();

		void clampBoundaries();
		void clampBoundaryPlane(size_t vertIdx);
		void clampBoundaryEdge(size_t vertIdx);
		void clampBoundaryCorner(size_t vertIdx);

		void minimizeMesh(int steps, int energyMask, const std::string& filename = "");

		double calVertEnergy(size_t vertIdx) const;

		void dumpModelObj(const string& filenameRoot) const;
		void dumpModelSharpEdgesObj(const string& filenameRoot, double sinAngle) const;

		ParamsRec _params;
		ReporterPtr _reporter;
		Grid _grid;
		DumpObj _dumpObj;
		std::vector<CModelPtr> _modelPtrs;
	};

	inline bool CMesher::modelExists(size_t modelIdx) const {
		return modelIdx < _modelPtrs.size();
	}
}
