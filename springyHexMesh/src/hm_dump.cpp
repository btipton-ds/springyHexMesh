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

#include <hm_dump.h>

#include <fstream>

#include <hm_gridVert.h>
#include <hm_gridEdge.h>
#include <hm_gridFace.h>
#include <hm_gridCell.h>

namespace HexahedralMesher {

	using namespace std;

	Dump::Dump(const Grid& grid, const std::string& path)
		: _grid(grid)
		, _path(path)
	{}

	void Dump::write(ostream& out, int minNumberOfClamps, int clampMask) const {
		vector<size_t> cellIndices;
		gatherClampedCells(minNumberOfClamps, clampMask, cellIndices);
		writeCells(out, cellIndices);
	}

	void Dump::write(const string& rootName, int minNumberOfClamps, int clampMask) const {
		string filename(rootName + ".obj");
		ofstream out(_path + filename);
		write(out, minNumberOfClamps, clampMask);
	}

	void Dump::writeFaces(std::ostream& out, const std::vector<GridFace>& faces) const {
		vector<size_t> vertIndices;
		map<size_t, size_t> vertIdxMap;
		createMaps(faces, vertIndices, vertIdxMap);

		set<GridEdge> edges;
		gatherEdges(faces, edges);

		set<SearchableFace> faceSet;
		gatherFaces(faces, faceSet);

		dumpVertsObj(out, vertIndices);
		dumpLinesObj(out, edges, vertIdxMap);
		dumpFacesObj(out, faceSet, vertIdxMap);
	}

	void Dump::writeFaces(std::ostream& out, int minNumberOfClamps, int clampMask) const {
		vector<GridFace> faces;
		gatherClampedFaces(minNumberOfClamps, clampMask, faces);
		writeFaces(out, faces);
	}

	void Dump::writeFaces(const std::string& rootName, int minNumberOfClamps, int clampMask) const {
		string filename(rootName + ".obj");
		ofstream out(_path + filename);
		writeFaces(out, minNumberOfClamps, clampMask);
	}

	void Dump::write(const std::string& rootName, const std::vector<Vector3d>& pts) const {
		string filename(rootName + ".obj");
		ofstream out(_path + filename);

		out << "#Vertices\n";
		out << "v " << 10 << " " << 10 << " " << 10 << "\n";
		for (const Vector3d& p : pts) {
			out << "v " << p[0] << " " << p[1] << " " << p[2] << "\n";
		}

		out << "#Edges\n";
		for (size_t i = 0; i < pts.size(); i++) {
			out << "l " << 1 << " " << (i + 2) << "\n";
		}

	}

	void Dump::writeCells(const std::string& filename, const std::vector<size_t>& cellIndices) const {
		string path = _path + filename;
		if (path.find(".obj") == string::npos)
			path += ".obj";
		ofstream out(path);
		writeCells(out, cellIndices);
	}

	void Dump::writeCells(ostream& out, const std::vector<size_t>& cellIndices) const {
		vector<size_t> vertIndices;
		map<size_t, size_t> vertIdxMap;
		createMaps(cellIndices, vertIndices, vertIdxMap);

		set<GridEdge> edges;
		gatherEdges(cellIndices, edges);

		set<SearchableFace> faceSet;
		gatherFaces(cellIndices, faceSet);

		dumpVertsObj(out, vertIndices);
		dumpLinesObj(out, edges, vertIdxMap);
		dumpFacesObj(out, faceSet, vertIdxMap);
	}

	void Dump::gatherClampedCells(int minNumberOfClamps, int clampMask, vector<size_t>& cellIndices) const {
		cellIndices.clear();

		_grid.iterateCells([&](size_t cellIdx)->bool {
			const auto& cell = _grid.getCell(cellIdx);
			int numClamps = 0;
			for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
				size_t vertIdx = cell.getVertIdx(p);
				const auto& vert = _grid.getVert(vertIdx);
				if (vert.getClamp().matches(clampMask))
					numClamps++;
			}

			if (numClamps >= minNumberOfClamps) {
				cellIndices.push_back(cellIdx);
			}
			return true;
		});
	}

	void Dump::gatherClampedFaces(int minNumberOfClamps, int clampMask, vector<GridFace>& faces) const {
		faces.clear();

		_grid.iterateCells([&](size_t cellIdx)->bool {
			const auto& cell = _grid.getCell(cellIdx);
			for (FaceNumber fn = BOTTOM; fn < FN_UNKNOWN; fn++) {
				int numClamps = 0;
				size_t idx[4];
				cell.getFaceIndices(fn, idx);
				for (int i = 0; i < 4; i++) {
					const auto& vert = _grid.getVert(idx[i]);
					if (vert.getClamp().matches(clampMask))
						numClamps++;
				}

				if (numClamps >= minNumberOfClamps) {
					faces.push_back(GridFace(cellIdx, fn));
				}
			}

			return true;
		});

	}

	void Dump::gatherEdges(const std::vector<size_t> cellIndices, std::set<GridEdge>& edges) const {
		for (size_t cellIdx : cellIndices) {
			const auto& cell = _grid.getCell(cellIdx);
			for (CellVertPos pos0 = LWR_FNT_LFT; pos0 < CVP_UNKNOWN; pos0++) {
				for (VertEdgeDir ed = X_POS; ed < VED_UNKNOWN; ed++) {
					CellVertPos pos1 = cell.getVertsEdgeEndPos(pos0, ed);
					if (pos1 != CVP_UNKNOWN) {
						size_t idx0 = cell.getVertIdx(pos0);
						size_t idx1 = cell.getVertIdx(pos1);
						edges.insert(GridEdge(idx0, idx1));

					}
				}
			}
		}
	}

	void Dump::gatherEdges(const std::vector<GridFace> faces, std::set<GridEdge>& edges) const {
		for (const auto& face : faces) {
			const auto& cell = _grid.getCell(face.getCellIdx());
			size_t idx[4];
			cell.getFaceIndices(face.getFaceNumber(), idx);
			for (int i = 0; i < 4; i++) {
				GridEdge edge(idx[i], idx[(i + 1) % 4]);
				edges.insert(edge);
			}
		}
	}

	void Dump::gatherFaces(const std::vector<size_t> cellIndices, std::set<SearchableFace>& faceSet) const {
		for (size_t cellIdx : cellIndices) {
			const GridCell& cell = _grid.getCell(cellIdx);
			for (FaceNumber fn = BOTTOM; fn < FN_UNKNOWN; fn++) {
				faceSet.insert(cell.getSearchableFace(fn));
			}
		}
	}

	void Dump::gatherFaces(const std::vector<GridFace> faces, std::set<SearchableFace>& faceSet) const {
		for (const GridFace& face: faces) {
			const GridCell& cell = _grid.getCell(face.getCellIdx());
			faceSet.insert(cell.getSearchableFace(face.getFaceNumber()));
		}
	}

	void Dump::createMaps(const vector<size_t>& cellIndices, vector<size_t>& vertIndices, map<size_t, size_t>& vertIdxMap) const {
		vertIndices.clear();
		vertIdxMap.clear();

		for (size_t cellIdx : cellIndices) {
			const auto& cell = _grid.getCell(cellIdx);
			for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
				size_t vertIdx = cell.getVertIdx(p);
				if (vertIdxMap.find(vertIdx) == vertIdxMap.end()) {
					size_t newIdx = vertIndices.size() + 1; // Obj is 1s indexed
					vertIndices.push_back(vertIdx);
					vertIdxMap.insert(make_pair(vertIdx, newIdx));
				}
			}
		}
	}

	void Dump::createMaps(const vector<GridFace>& faces, vector<size_t>& vertIndices, map<size_t, size_t>& vertIdxMap) const {
		vertIndices.clear();
		vertIdxMap.clear();

		for (const auto& face : faces) {
			const auto& cell = _grid.getCell(face.getCellIdx());
			size_t idx[4];
			cell.getFaceIndices(face.getFaceNumber(), idx);
			for (int i = 0; i < 4; i++) {
				size_t vertIdx = idx[i];
				if (vertIdxMap.find(vertIdx) == vertIdxMap.end()) {
					size_t newIdx = vertIndices.size() + 1; // Obj is 1s indexed
					vertIndices.push_back(vertIdx);
					vertIdxMap.insert(make_pair(vertIdx, newIdx));
				}
			}
		}
	}

	void Dump::dumpVertsObj(ostream& out, const vector<size_t>& vertIndices) const {
		out << "#Vertices\n";
		for (size_t oldIdx : vertIndices) {
			const auto& vert = _grid.getVert(oldIdx);
			const auto& p = vert.getPt();
			out << "v " << p[0] << " " << p[1] << " " << p[2];

			switch (vert.getClampType())
			{
			case CLAMP_FIXED:
				out << " 0.5 0.5 1";
				break;
			case CLAMP_PARALLEL:
				out << " 1 0.5 0.5";
				break;
			case CLAMP_PERPENDICULAR:
				out << " 0.5 1 0.5";
				break;
			case CLAMP_VERT:
				out << " 1 0 0";
				break;
			case CLAMP_EDGE:
				out << " 0 1 0";
				break;
			case CLAMP_TRI:
				out << " 0 0 1";
				break;
			case CLAMP_CELL_EDGE_CENTER:
				out << " 1 0 1";
				break;
			case CLAMP_CELL_FACE_CENTER:
				out << " 1 1 0";
				break;
			case CLAMP_GRID_TRI_PLANE:
				out << " 1 1 0.5";
				break;
			default:
				out << " 1 1 1";
				break;
			}
			out << "\n";
		}
	}

	void Dump::dumpLinesObj(ostream& out, const set<GridEdge>& edges, const map<size_t, size_t>& vertIdxMap) const {
		out << "#Edges\n";
		for (const GridEdge& edge : edges) {
			auto iter0 = vertIdxMap.find(edge.getVert(0));
			auto iter1 = vertIdxMap.find(edge.getVert(1));
			if (iter0 != vertIdxMap.end() && iter1 != vertIdxMap.end())
				out << "l " << iter0->second << " " << iter1->second << "\n";
		}
	}

	void Dump::dumpFacesObj(ostream& out, const set<SearchableFace>& faceSet, const map<size_t, size_t>& vertIdxMap) const {
		out << "#Faces\n";
		for (const SearchableFace& face : faceSet) {
			const auto& cell = _grid.getCell(face.getCellIdx());
			size_t idx[4];
			cell.getFaceIndices(face.getFaceNumber(), idx);
			out << "f ";
			for (int i = 0; i < 4; i++) {
				auto iter = vertIdxMap.find(idx[i]);
				out << iter->second << " ";
			}
			out << "\n";

		}
	}

	void Dump::dumpCellObj(string& filename, size_t cellIdx) const {
		string path("C:\\Users\\Bob\\Documents\\Projects\\ElectroFish\\HexMeshTests\\");
		ofstream out(path + filename + ".obj");
		dumpCellObj(out, cellIdx);
	}

	void Dump::dumpCellObj(ostream& out, size_t cellIdx) const {
		const GridCell& cell = _grid.getCell(cellIdx);
		Vector3d pts[8];
		for (CellVertPos p = LWR_FNT_LFT; p < CVP_UNKNOWN; p++) {
			pts[p] = cell.getVert(_grid, p).getPt();
		}
		out << "#Vertices\n";
		for (int i = 0; i < 8; i++) {
			const auto& p = pts[i];
			out << "v " << p[0] << " " << p[1] << " " << p[2] << "\n";
		}

		out << "#Faces\n";
		for (FaceNumber fn = BOTTOM; fn < FN_UNKNOWN; fn++) {
			CellVertPos facePos[4];
			GridCell::getFaceCellPos(fn, facePos);
			out << "f " << ((int)facePos[0] + 1) << " " << ((int)facePos[1] + 1) << " " << ((int)facePos[2] + 1) << " " << ((int)facePos[3] + 1) << "\n";
		}

	}


}