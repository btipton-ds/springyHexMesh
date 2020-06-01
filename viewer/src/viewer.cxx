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
#include <fstream>

#include <meshProcessor.h>

using namespace std;
using namespace HexahedralMesher;

class TestReporter : public CMesher::Reporter {
public:
	void report(const CMesher& mesher, const std::string& key) const override {

	}

};

int main(int numArgs, char** args)
{
	string downloads = "../../../../test_data/";
	ParamsRec params;

	params.bounds.clear();
	params.bounds.merge(Vector3d(-2, 0, 0));
	params.bounds.merge(Vector3d(6, 6, 16));
	params.maxEdgeLength = 1;
	params.minEdgeLength = 0.1;
	params.sharpAngleDeg = 45.0;

	TestReporter reporter;
	CMesher mesher(params, reporter);
	mesher.reset();
	bool fine = false;
	if (!mesher.addFile(downloads, fine ? "Spinnaker Slots 5 - Fine.stl" : "Spinnaker Slots 5 - Coarse.stl"))
		return 1;
	mesher.run();

	return 0;
}
