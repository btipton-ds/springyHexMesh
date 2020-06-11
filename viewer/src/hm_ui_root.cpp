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

#include <vk_defines.h>

#include <vk_ui_button.h>
#include <vk_ui_window.h>
#include <vk_model.h>
#include <vk_pipeline3D.h>
#include <hm_ui_modelEdges.h>
#include <vk_app.h>

#include <hm_ui_gridIndexNode.h>
#include <hm_ui_root.h>

using namespace std;
using namespace HexahedralMesher;
using namespace UI;
using Button = VK::UI::Button;
using ButtonPtr = VK::UI::ButtonPtr;
using Rect = VK::UI::Rect;

namespace {

	struct comparePosNormFunc {
		inline bool operator()(const VK::Vertex3_PNCf& lhs, const VK::Vertex3_PNCf& rhs) const {
			for (int i = 0; i < 3; i++) {
				if (lhs.pos[i] < rhs.pos[i])
					return true;
				else if (lhs.pos[i] > rhs.pos[i])
					return false;
			}

			for (int i = 0; i < 3; i++) {
				if (fabs(lhs.norm[i]) < fabs(rhs.norm[i]))
					return true;
				else if (fabs(lhs.norm[i]) > fabs(rhs.norm[i]))
					return false;
			}
			return false;
		}
	};

	struct SearchableTri {
		inline SearchableTri(const array<uint32_t, 3>& tri) {
			_idx = tri;
			sort(_idx.begin(), _idx.end());
		}

		inline bool operator < (const SearchableTri& lhs) const {
			for (size_t i = 0; i < 3; i++) {
				if (_idx[i] < lhs._idx[i])
					return true;
				else if (_idx[i] > lhs._idx[i])
					return false;
			}
			return false;
		}

		array<uint32_t, 3> _idx;
	};

	glm::vec3 colorOf(const GridVert& vert) {
		glm::vec3 color;
		switch (vert.getClampType()) {
		default:
			color = { 1, 1, 1 };
			break;
		case CLAMP_FIXED:
			color = { 0.5f, 0.5f, 1.0f };
			break;
		case CLAMP_PARALLEL:
			color = { 1.0f, 0.5f, 0.5f };
			break;
		case CLAMP_PERPENDICULAR:
			color = { 0.5f, 1.0f, 0.5f };
			break;
		case CLAMP_VERT:
			color = { 1.0f, 0.0f, 0.0f };
			break;
		case CLAMP_EDGE:
			color = { 0.0f, 1.0f, 0.0f };
			break;
		case CLAMP_TRI:
			color = { 0.0f, 0.0f, 1.0f };
			break;
		case CLAMP_CELL_EDGE_CENTER:
			color = { 1.0f, 0.0f, 1.0f };
			break;
		case CLAMP_CELL_FACE_CENTER:
			color = { 1.0f, 1.0f, 0.0f };
			break;
		case CLAMP_GRID_TRI_PLANE:
			color = { 1.0f, 1.0f, 0.5f };
			break;
		}

		return color;
	}
}

Root::Root(const CMesherPtr& mesher)
	: _mesher(mesher)
{
	_app = make_shared<VK::VulkanApp>(1500, 900);
	glfwSetWindowTitle(_app->getWindow(), "Springy Hex Mesh");
	_app->setAntiAliasSamples(VK_SAMPLE_COUNT_4_BIT);

	VK::UI::WindowPtr gui = make_shared<VK::UI::Window>(_app);
	_app->setUiWindow(gui);
	buildUi(gui);

	_plShaded = _app->addPipelineWithSource<VK::Pipeline3D>("model_shaded", "shaders/shader_vert.spv", "shaders/shader_frag.spv");

	// _plFaceBounds uses a special solid color shader for the edges. It could be combined with _plEdges if the shader is modified.
	_plFaceBounds = _app->addPipelineWithSource<VK::Pipeline3D>("grid_face_bounds", "shaders/shader_grid_wireframe_vert.spv", "shaders/shader_grid_wireframe_frag.spv");
	_plFaceBounds->setPolygonMode(VK_POLYGON_MODE_LINE);
	_plFaceBounds->setToplogy(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
	_plFaceBounds->setLineWidth(0.25);

	_plTriWire = _app->addPipelineWithSource<VK::Pipeline3D>("model_wireframe", "shaders/shader_vert.spv", "shaders/shader_wireframe_frag.spv");
	_plTriWire->setPolygonMode(VK_POLYGON_MODE_LINE);
	_plTriWire->toggleVisiblity();

	_plEdges = _app->addPipelineWithSource<VK::Pipeline3D>("model_edges", "shaders/shader_vert.spv", "shaders/shader_wireframe_frag.spv");
	_plEdges->setPolygonMode(VK_POLYGON_MODE_LINE);
	_plEdges->setToplogy(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
	_plEdges->setDepthTestEnabled(false);
//	_plEdges->toggleVisiblity();
}

Root::~Root() {
}


void Root::buildUi(const VK::UI::WindowPtr& win) {
	using namespace glm;

	vec4 bkgColor(0.875f, 0.875f, 0.875f, 1);
	uint32_t w = 200;
	uint32_t h = 24;
	uint32_t row = 0;

	addViewButtons(win);

	row += h;
	win->addButton(bkgColor, "Toggle Model", Rect(row, 0, row + h, w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			_models.front()._sceneNodeShaded->toggleVisibility();
		}
	});

	row += h;
	win->addButton(bkgColor, "Toggle Model Edges", Rect(row, 0, row + h, w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			_plEdges->toggleVisiblity();
		}
	});

	row += h;
	win->addButton(bkgColor, "Toggle Grid", Rect(row, 0, row + h, w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			_allCells.toggleVisibility();
		}
	});

	row += h;
	win->addButton(bkgColor, "Toggle Grid Shaded", Rect(row, 0, row + h, w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			_allCells.toggleMode();
		}
	});

	row += h;
	win->addButton(bkgColor, "Toggle C-Cells-1", Rect(row, 0, row + h, w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			_clampedCells1.toggleVisibility();
		}
	});

	row += h;
	win->addButton(bkgColor, "Toggle C-Cells-1 Shaded", Rect(row, 0, row + h, w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			_clampedCells1.toggleMode();
		}
	});

	row += h;
	win->addButton(bkgColor, "Screenshot", Rect(row, 0, row + h, w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			const auto& swapChain = _app->getSwapChain();
			const auto& images = swapChain._images;
			const auto& image = images[_app->getSwapChainIndex()];

			string path = "/Users/Bob/Documents/Projects/ElectroFish/HexMeshTests/";
			image->saveImage(path + "screenshot.bmp");
			image->saveImage(path + "screenshot.jpg");
			image->saveImage(path + "screenshot.png");
		}
	});

	row += h;
	win->addButton(bkgColor, "Quit", Rect(row, 0, row + h, w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			_isRunning = false; // This stops the mesher
			_app->stop();
		}
	});

}

void Root::addViewButtons(const VK::UI::WindowPtr& win) {
	using namespace glm;

	vec4 bkgColor(0.875f, 0.875f, 0.875f, 1);
	uint32_t w = 60;
	uint32_t h = 24;
	uint32_t row = 0;
	uint32_t col = 200;

	win->addButton(bkgColor, "X+", Rect(row, col, row + h, col + w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			mat3 xform = mat3(
				vec3(1, 0, 0),
				vec3(0, 1, 0),
				vec3(0, 0, 1)
			);
			_app->setModelToWorldTransform(xform);
		}
	});

	col += w;
	win->addButton(bkgColor, "X-", Rect(row, col, row + h, col + w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			mat3 xform = mat3(
				vec3(-1, 0, 0),
				vec3(0, -1, 0),
				vec3(0, 0, 1)
			);
			_app->setModelToWorldTransform(xform);
		}
	});

	col += w;
	win->addButton(bkgColor, "Y+", Rect(row, col, row + h, col + w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			mat3 xform = mat3(
				vec3(0, 0, 1),
				vec3(1, 0, 0),
				vec3(0, 1, 0)
			);
			_app->setModelToWorldTransform(xform);
		}
	});

	col += w;
	win->addButton(bkgColor, "Y-", Rect(row, col, row + h, col + w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			mat3 xform = mat3(
				vec3(0, 0, 1),
				vec3(-1, 0, 0),
				vec3(0, -1, 0)
			);
			_app->setModelToWorldTransform(xform);
		}
	});

	col += w;
	win->addButton(bkgColor, "Z+", Rect(row, col, row + h, col + w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			mat3 xform = mat3(
				vec3(0, 1, 0),
				vec3(0, 0, 1),
				vec3(1, 0, 0)
			);
			_app->setModelToWorldTransform(xform);
		}
	});

	col += w;
	win->addButton(bkgColor, "Z-", Rect(row, col, row + h, col + w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			mat3 xform = mat3(
				vec3(0, -1, 0),
				vec3(0, 0, -1),
				vec3(-1, 0, 0)
			);
			_app->setModelToWorldTransform(xform);
		}
	});

}

bool Root::run() {
	try {
		_app->run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return false;
	}
	catch (...) {
		std::cerr << "unknown error\n";
	}

	return true;
}

inline void Root::waitTillClear(size_t bits) const {
	while (_updateBits & bits) {
		this_thread::sleep_for(chrono::milliseconds(100));
	}
}

inline void Root::setState(size_t bits) {
	_updateBits |= STATE_PENDING | bits;
	waitTillClear(STATE_PENDING);
}

void Root::report(const CMesher& mesher, const std::string& key) {
	waitTillClear(STATE_WORKING);

	if (key == "grid_topol_change") {
		setState(STATE_TOPOL_CHANGED);
	} else if (key == "grid_verts_changed") {
		setState(STATE_VERTS_UDATED);
	}
}

void Root::updateVkApp() {
	if (_updateBits & STATE_TOPOL_CHANGED) {
		_updateBits = STATE_WORKING;

		buildBuffers();

		_updateBits = STATE_IDLE;
	} else if (_updateBits & STATE_VERTS_UDATED) {
		_updateBits = STATE_WORKING;

		updateVerts();

		_updateBits = STATE_IDLE;
	}
}

void Root::buildBuffers() {
	const auto& grid = *_mesher->getGrid();
	if (grid.numCells() == 0)
		return;

	buildPosNormBuffer(true);
	addGridFaces();

	addClampedCells();
}

void Root::addClampedCells() {
	const auto& grid = *_mesher->getGrid();
	Dump dump(grid);

	{
		vector<size_t> cellIds;
		dump.gatherClampedCells(1, CLAMP_VERT | CLAMP_EDGE | CLAMP_TRI, cellIds);

		vector<uint32_t> tris;
		vector<uint32_t> quadEdges;

		for (size_t cellId : cellIds) {
			addCellToDrawLists(cellId, tris, quadEdges);
		}

		create(_clampedCells1, tris, quadEdges);
	}
}

void Root::create(GridDrawSet& gds, const std::vector<uint32_t>& tris, const std::vector<uint32_t>& edges) {
	gds._shaded._tris = make_shared<GridIndexNode>(this, _plShaded);
	gds._shaded._tris->setFaceDrawList(tris);
	_plShaded->addSceneNode(gds._shaded._tris);

	gds._shaded._bounds = make_shared<GridIndexNode>(this, _plFaceBounds);
	gds._shaded._bounds->setFaceDrawList(edges);
	_plFaceBounds->addSceneNode(gds._shaded._bounds);

	gds._edges = make_shared<GridIndexNode>(this, _plEdges);
	gds._edges->setFaceDrawList(edges);
	gds._edges->toggleVisibility();
	_plEdges->addSceneNode(gds._edges);

}

void Root::buildPosNormBuffer(bool buildTopology) {
	const auto& grid = *_mesher->getGrid();

	_bbox.clear();

	if (buildTopology) {
		_tris.clear();
		_faceToTriMap.clear();
	}

	vector<VK::Vertex3_PNCf> verts;
	map<SearchableTri, size_t> triMap;
	set<SearchableFace> faceSet;

	grid.iterateCells([&](size_t cellId)->bool {
		const auto& cell = grid.getCell(cellId);

		for (FaceNumber fn = BOTTOM; fn < FN_UNKNOWN; fn++) {
			GridFace face(cellId, fn);
			const Vector3d* triPtsArr[2][3];
			face.getTriVertPtrs(grid, triPtsArr);

			size_t triIdxArr[2][3];
			face.getTriIndices(grid, triIdxArr);

			array<size_t, 2> faceTris;
			for (int i = 0; i < 2; i++) {
				const auto triPts = triPtsArr[i];
				const auto triIdx = triIdxArr[i];
				auto norm = triangleNormal(triPts);

				array<uint32_t, 3> tri;
				for (int j = 0; j < 3; j++) {
					size_t vertIdx = triIdx[j];
					const auto& gVert = grid.getVert(vertIdx);
					VK::Vertex3_PNCf vert;
					const auto& pt = gVert.getPt();
					vert.pos = { (float)pt[0], (float)pt[1], (float)pt[2] };

					vert.norm = { (float)norm[0], (float)norm[1], (float)norm[2] };
					vert.norm = glm::normalize(vert.norm);

					vert.color = colorOf(gVert);

					size_t idx = verts.size();
					verts.push_back(vert);
					_bbox.merge(Vector3f(vert.pos[0], vert.pos[1], vert.pos[2]));

					tri[j] = (uint32_t)idx;
				}

				if (buildTopology) {
					SearchableTri st(tri);
					auto iter = triMap.find(st);
					if (iter == triMap.end()) {
						size_t idx = _tris.size();
						_tris.push_back(tri);
						iter = triMap.insert(make_pair(st, idx)).first;
					}
					faceTris[i] = iter->second;
				}
			}

			if (buildTopology) {
				SearchableFace sf(face.getSearchableFace(grid));

				_faceToTriMap.insert(make_pair(sf, faceTris));
			}
		}

		return true;
	});

	constexpr VkBufferUsageFlags bufUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	constexpr VkMemoryPropertyFlags bufProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	if (!_posNormVertBuffer) {
		_posNormVertBuffer = make_shared<VK::Buffer>(_app->getDeviceContext());
		_posNormVertBuffer->create(verts, bufUsageFlags, bufProperties);
	} else if (_posNormVertBuffer->getSize() == verts.size() * sizeof(verts[0])) {
		_posNormVertBuffer->updateVec(verts);
	} else {
		_posNormVertBuffer = make_shared<VK::Buffer>(_app->getDeviceContext());
		_posNormVertBuffer->create(verts, bufUsageFlags, bufProperties);
	}

}

void Root::updateVerts() {
	buildPosNormBuffer(false);
}

void Root::addCellToDrawLists(size_t cellId, vector<uint32_t>& tris, vector<uint32_t>& quadEdges) {
	const Grid& grid = *_mesher->getGrid();
	const auto& cell = grid.getCell(cellId);
	for (FaceNumber fn = BOTTOM; fn < FN_UNKNOWN; fn++) {
		SearchableFace sf(cell.getSearchableFace(fn));
		auto iter = _faceToTriMap.find(sf);
		if (iter != _faceToTriMap.end()) {
			const auto& triPair = iter->second;
			const auto& tri0 = _tris[triPair[0]];
			const auto& tri1 = _tris[triPair[1]];
			for (size_t idx : tri0) {
				tris.push_back((uint32_t)idx);
			}
			for (size_t idx : tri1) {
				tris.push_back((uint32_t)idx);
			}

			quadEdges.push_back(tri0[0]);
			quadEdges.push_back(tri0[1]);

			quadEdges.push_back(tri0[1]);
			quadEdges.push_back(tri0[2]);

			quadEdges.push_back(tri1[1]);
			quadEdges.push_back(tri1[2]);

			quadEdges.push_back(tri1[2]);
			quadEdges.push_back(tri1[0]);
		}
	}
}


void Root::addGridFaces() {
	vector<uint32_t> tris;
	vector<uint32_t> quadEdges;

	const Grid& grid = *_mesher->getGrid();

	grid.iterateCells([&](size_t cellId)->bool { 
		addCellToDrawLists(cellId, tris, quadEdges);
		return true;
	});

	_allCells._shaded._tris = make_shared<GridIndexNode>(this, _plShaded);
	_allCells._shaded._tris->setFaceDrawList(tris);
	_plShaded->addSceneNode(_allCells._shaded._tris);

	_allCells._shaded._bounds = make_shared<GridIndexNode>(this, _plFaceBounds);
	_allCells._shaded._bounds->setFaceDrawList(quadEdges);
	_plFaceBounds->addSceneNode(_allCells._shaded._bounds);

	_allCells._edges = make_shared<GridIndexNode>(this, _plEdges);
	_allCells._edges->setFaceDrawList(quadEdges);
	_plEdges->addSceneNode(_allCells._edges);

	_allCells.restoreVisibility();
}

void Root::reportModelAdded(const CMesher& mesher, const CModelPtr& model) {
	VK::ModelPtr uiModelShaded = VK::Model::create(_plShaded, model, glm::vec3(0.5f, 0.5f, 0.75f));
	_plShaded->addSceneNode(uiModelShaded);

	VK::ModelPtr uiModelWf = VK::Model::create(_plTriWire, model);
	_plTriWire->addSceneNode(uiModelWf);

	ModelEdgesPtr uiModelEdges = ModelEdges::create(_plEdges, model);
	_plEdges->addSceneNode(uiModelEdges);

	ModelRec rec = { model, uiModelShaded, uiModelWf, uiModelEdges };

	_models.push_back(rec);
}

bool Root::isRunning() const {
	return _isRunning;
}
