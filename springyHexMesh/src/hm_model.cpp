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

#include <hm_model.h>

#include <set>

namespace HexahedralMesher {

	using namespace std;

	CModel::CModel(double sharpAngleDeg)
		: _sinSharpAngle(sin(sharpAngleDeg* EIGEN_PI / 180.0))
	{

	}

	void CModel::findSharpEdges(std::vector<size_t>& sharps) {
		BoundingBox sharpBox;

		for (size_t edgeIdx = 0; edgeIdx < numEdges(); edgeIdx++) {
			if (isEdgeSharp(edgeIdx, _sinSharpAngle)) {
				const auto& edge = getEdge(edgeIdx);
				sharpBox.merge(getVert(edge._vertIndex[0])._pt);
				sharpBox.merge(getVert(edge._vertIndex[1])._pt);
				sharps.push_back(edgeIdx);
			}
		}

		_sharpEdgeTree.reset(sharpBox);
		_cusps.clear();

		for (const auto& edgeIdx : sharps) {
			const auto& edge = getEdge(edgeIdx);

			BoundingBox edgeBB;
			edgeBB.merge(getVert(edge._vertIndex[0])._pt);
			edgeBB.merge(getVert(edge._vertIndex[1])._pt);

			_sharpEdgeTree.add(edgeBB, edgeIdx);
		}

	}

	void CModel::createPolylines(std::vector<size_t>& sharps) {
		while (!sharps.empty()) {
			bool added = false;
			for (size_t i = 0; i < sharps.size(); i++) {
				const auto& edge = getEdge(sharps[i]);
				for (auto& pl : _polyLines) {
					if (pl.addEdgeToLine(edge)) {
						added = true;
						sharps.erase(sharps.begin() + i);
						break;
					}
				}
				if (added)
					break;
			}
			if (!added) {
				const auto& edge = getEdge(sharps.back());
				_polyLines.push_back(TriMesh::CPolyLine());
				_polyLines.back().addEdgeToLine(edge);
				sharps.pop_back();
			}
		}
	}

	void CModel::createCuspsAndSplitPolylines() {
		const double cosAngle = sqrt(1 - _sinSharpAngle * _sinSharpAngle);
		vector<TriMesh::CPolyLine> temp(_polyLines);
		_polyLines.clear();

		for (const auto& pl : temp) {
			set<size_t> cuspIndexSet;
			const auto& verts = pl.getVerts();
			size_t numSegs = verts.size() - 1;

			if (pl.isClosed())
				numSegs++;
			if (numSegs < 3)
				continue;

			if (!pl.isClosed()) {
				// Add the start and end of an open polyline
				size_t vertIdx;

				vertIdx = verts.front();
				_cusps.insert(vertIdx);
				cuspIndexSet.insert(0);

				vertIdx = verts.back();
				_cusps.insert(vertIdx);
				cuspIndexSet.insert(verts.size() - 1);
			}

			if (numSegs < 3)
				continue;

			for (size_t i = 0; i < numSegs; i++) {
				// Add the vertex of a cusp corner in a closed polyline
				size_t j = (i + 1) % verts.size();
				size_t k = (i + 2) % verts.size();

				size_t vertIdx0 = verts[i];
				size_t vertIdx1 = verts[j];
				size_t vertIdx2 = verts[k];

				const Vector3d& pt0 = getVert(vertIdx0)._pt;
				const Vector3d& pt1 = getVert(vertIdx1)._pt;
				const Vector3d& pt2 = getVert(vertIdx2)._pt;
				Vector3d v0 = (pt1 - pt0).normalized();
				Vector3d v1 = (pt2 - pt1).normalized();

				double dp = v0.dot(v1);
				if (dp < cosAngle) {
					_cusps.insert(verts[j]);
					cuspIndexSet.insert(j);
				}
			}

			vector<size_t> cuspIndices;
			cuspIndices.insert(cuspIndices.end(), cuspIndexSet.begin(), cuspIndexSet.end());

			for (size_t ci = 0; ci < cuspIndices.size(); ci++) {
				size_t cj = (ci + 1) % cuspIndices.size();
				TriMesh::CPolyLine pl2;
				size_t startIdx = cuspIndices[ci];
				size_t endIdx = cuspIndices[cj];
				if (endIdx < startIdx) {
					endIdx += verts.size();
				}
				for (size_t i = startIdx; i < endIdx; i++) {
					TriMesh::CEdge edge(verts[i % verts.size()], verts[(i + 1) % verts.size()]);
					pl2.addEdgeToLine(edge);
				}
				_polyLines.push_back(pl2);
			}

			int dbgBreak = 1;
		}
	}

	void CModel::init() {

		vector<size_t> sharps;
		findSharpEdges(sharps);
		createPolylines(sharps);
		createCuspsAndSplitPolylines();

	}

	void CModel::save(ostream& out) const {
		out << "Model version 1\n";

		TriMesh::CMesh::save(out);

		out << "#PL " << _polyLines.size() << "\n";
		for (const auto& pl : _polyLines)
			pl.save(out);
	}

	bool CModel::read(istream& in) {
		string str0, str1;
		int version;
		in >> str0 >> str1 >> version;
		if (str0 != "Model" || str1 != "version")
			return false;

		if (!TriMesh::CMesh::read(in))
			return false;

		size_t numPolylines;
		in >> str0 >> numPolylines;
		if (str0 != "#PL")
			return false;

		_polyLines.resize(numPolylines);
		for (auto& pl : _polyLines) {
			if (!pl.read(in))
				return false;
		}

		return true;
	}

}