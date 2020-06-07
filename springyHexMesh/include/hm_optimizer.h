#pragma once

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

#include <tm_defines.h>
#include <tm_math.h>

namespace HexahedralMesher {

	template<class VECTOR_TYPE>
	class SteepestAcent {
	public:
		SteepestAcent(double minVal, double differential = 1.0e-8)
			: _dt(differential)
			, _minVal(minVal)
		{}

		template<typename VAL_FUNC>
		static double calcMoveDist (VECTOR_TYPE& curValue, double dt, const Vector3d& gradient, VAL_FUNC calVal) {
			double val1 = calVal(curValue);
			if (fabs(val1) < minNormalizeDivisor)
				return 0;

			double val0 = calVal(curValue - dt * gradient) - val1;
			double val2 = calVal(curValue + dt * gradient) - val1;

			double a = (val2 + val0) / (2 * dt * dt);
			if (fabs(a) < minNormalizeDivisor)
				return 0;

			double b = (val2 - val0) / (2 * dt);
			double moveDist = -b / (2 * a);

			if (std::isnan(moveDist) || std::isinf(moveDist)) {
				calVal(curValue);
				throw "Bad moveDist";
			}
			return moveDist;
		};

		template<typename VAL_FUNC, typename GRAD_FUNC, typename LOG_FUNC>
		double run(VECTOR_TYPE& curValue, int maxSteps, double maxChange, VAL_FUNC calVal, GRAD_FUNC calGrad, LOG_FUNC log) {
			VECTOR_TYPE startPoint = curValue;
			double moveDist = DBL_MAX;
			double maxStep = 0.2 * maxChange;
			for (int count = 0; count < maxSteps && moveDist > OPTIMIZER_TOL; count++) {
				VECTOR_TYPE gradient;
				double maxDist = calGrad(_dt, gradient);
				if (maxDist == 0) {
					// This only returned when calGrad couldn't find an increasing direction
					break;
				}

				moveDist = calcMoveDist(curValue, _dt, gradient, calVal);
				if (moveDist > maxStep)
					moveDist = maxStep;

				if (moveDist > maxDist) {
					// This is the faceted move limit, we've run onto a dicontinuity like a vertext or
					// edge on the model. Let calGrad test if we can progress.
					curValue = curValue + maxDist * gradient;
					continue;
				}

				VECTOR_TYPE nextPoint = curValue + moveDist * gradient;
				double totalDist = (nextPoint - startPoint).norm();
				if (totalDist > maxChange) {
					break;
				}
				curValue = curValue + moveDist * gradient;
				log(count, moveDist);
			}

			moveDist = (curValue - startPoint).norm();
			return (curValue - startPoint).norm();
		}

	private:
		double _dt, _minVal;
	};
}
