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

#include <hm_ui_root.h>

using namespace std;
using namespace HexahedralMesher;
using namespace UI;
using Button = VK::UI::Button;
using ButtonPtr = VK::UI::ButtonPtr;
using Rect = VK::UI::Rect;

Root::Root(const CMesherPtr& mesher)
	: _mesher(mesher)
{
	_app = make_shared<VK::VulkanApp>(1500, 900);
	glfwSetWindowTitle(_app->getWindow(), "Springy Hex Mesh");

	VK::UI::WindowPtr gui = make_shared<VK::UI::Window>(_app);
	_app->setUiWindow(gui);
	buildUi(gui);

	_pipelineStlShaded = _app->addPipelineWithSource<VK::Pipeline3D>("model_shaded", "shaders/shader_vert.spv", "shaders/shader_frag.spv");

	_pipelineStlWireframe = _app->addPipelineWithSource<VK::Pipeline3D>("model_wireframe", "shaders/shader_vert.spv", "shaders/shader_wireframe_frag.spv");
	_pipelineStlWireframe->setPolygonMode(VK_POLYGON_MODE_LINE);
	_pipelineStlWireframe->toggleVisiblity();

	_pipelineEdges = _app->addPipelineWithSource<VK::Pipeline3D>("model_edges", "shaders/shader_vert.spv", "shaders/shader_wireframe_frag.spv");
	_pipelineEdges->setPolygonMode(VK_POLYGON_MODE_LINE);
	_pipelineEdges->setToplogy(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
	_pipelineEdges->setDepthTestEnabled(false);
//	_pipelineEdges->toggleVisiblity();
}

Root::~Root() {
}


void Root::buildUi(const VK::UI::WindowPtr& win) {
	glm::vec4 bkgColor(0.875f, 0.875f, 0.875f, 1);
	uint32_t w = 200;
	uint32_t h = 24;
	uint32_t row = 0;

	win->addButton(bkgColor, "Reset View", Rect(row, 0, row + h, w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			glm::mat4 xform = glm::mat4(1.0f);
			_app->setModelToWorldTransform(xform);
		}
	});

	row += h;
	win->addButton(bkgColor, "Toggle shaded", Rect(row, 0, row + h, w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			if (!_models.empty()) {
				_models.front()._sceneNodeShaded->toggleVisibility();
			}
		}
	});

	row += h;
	win->addButton(bkgColor, "Toggle Wireframe", Rect(row, 0, row + h, w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			_pipelineStlWireframe->toggleVisiblity();
		}
	});

	row += h;
	win->addButton(bkgColor, "Toggle Edges", Rect(row, 0, row + h, w))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			if (!_models.empty()) {
				_pipelineEdges->toggleVisiblity();
			}
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

void Root::report(const CMesher& mesher, const std::string& key) const {

}

void Root::reportModelAdded(const CMesher& mesher, const CModelPtr& model) {
	VK::ModelPtr uiModelShaded = VK::Model::create(_pipelineStlShaded, model);
	_pipelineStlShaded->addSceneNode(uiModelShaded);

	VK::ModelPtr uiModelWf = VK::Model::create(_pipelineStlWireframe, model);
	_pipelineStlWireframe->addSceneNode(uiModelWf);

	ModelEdgesPtr uiModelEdges = ModelEdges::create(_pipelineEdges, model);
	_pipelineEdges->addSceneNode(uiModelEdges);

	ModelRec rec = { model, uiModelShaded, uiModelWf, uiModelEdges };

	_models.push_back(rec);
}

bool Root::isRunning() const {
	return _isRunning;
}
