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

#include <vk_defines.h>

#include <vk_ui_button.h>
#include <vk_ui_window.h>
#include <vk_app.h>

#include <hm_ui_root.h>

using namespace std;
using namespace HexahedralMesher;
using namespace UI;
using Button = VK::UI::Button;
using ButtonPtr = VK::UI::ButtonPtr;
using Rect = VK::UI::Rect;

Root::Root() {
	_app = make_shared<VK::VulkanApp>(1500, 900);
	glfwSetWindowTitle(_app->getWindow(), "Springy Hex Mesh");

	VK::UI::WindowPtr gui = make_shared<VK::UI::Window>(_app);
	_app->setUiWindow(gui);
	buildUi(gui);
}

Root::~Root() {
}

void Root::buildUi(const VK::UI::WindowPtr& win) {
	glm::vec4 bkgColor(0.875f, 0.875f, 0.875f, 1);
	uint32_t w = 120;
	uint32_t h = 22;
	uint32_t row = 0;

	win->addButton(Button(_app, bkgColor, "Reset View", Rect(row, 0, row + h, w)))->
		setAction(Button::ActionType::ACT_CLICK, [&](int btnNum, int modifiers) {
		if (btnNum == 0) {
			glm::mat4 xform = glm::mat4(1.0f);
			_app->setModelToWorldTransform(xform);
		}
	});

	row += h;
	win->addButton(Button(_app, bkgColor, "Screenshot", Rect(row, 0, row + h, w)))->
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
	win->addButton(Button(_app, bkgColor, "Button 2", Rect(row, 0, row + h, w)));
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
	_models.push_back(model);

	auto meshModel = _app->addSceneNode3D(model);
}
