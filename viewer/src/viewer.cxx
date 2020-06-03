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

#include <iostream>
#include <fstream>

#include <meshProcessor.h>

#include <fstream>

#include "vk_app.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vk_model.h"
#include "vk_modelObj.h"
#include <vk_pipelineUi.h>
#include <vk_ui_button.h>
#include <vk_ui_window.h>

#include <triMesh.h>
#include <readStl.h>

#include <hm_ui_root.h>

using namespace std;
using namespace HexahedralMesher;

int main(int numArgs, char** args)
{
	string downloads = "../../../../test_data/";

	UI::RootPtr uiRoot = make_shared<UI::Root>();
	
	ParamsRec params;

	params.bounds.clear();
	params.bounds.merge(Vector3d(-2, 0, 0));
	params.bounds.merge(Vector3d(6, 6, 16));
	params.maxEdgeLength = 1;
	params.minEdgeLength = 0.1;
	params.sharpAngleDeg = 45.0;

	CMesherPtr mesher = make_shared<CMesher>(params, uiRoot);

	mesher->reset();
	bool fine = true;
	if (!mesher->addFile(downloads, fine ? "Spinnaker Slots 5 - Fine.stl" : "Spinnaker Slots 5 - Coarse.stl"))
		return 1;

	// GLFW events only work from the main thread. So the GUI needs to be the main thread.
//	mesher->runAsThread();
	uiRoot->run();

	return 0;
}
