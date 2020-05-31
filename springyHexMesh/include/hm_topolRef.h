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

#include <tm_defines.h>

#include <iostream>

#include <tm_math.h>
#include <hm_types.h>
#include <hm_gridEdge.h>
#include <hm_gridFace.h>

namespace HexahedralMesher {

	class TopolRef {
	public:
		static TopolRef createNone();
		static TopolRef createFixed();
		static TopolRef createPerpendicular(const Vector3d& v);
		static TopolRef createParallel(const Vector3d& v);
		static TopolRef createVert(size_t meshIdx, size_t vertIdx);
		static TopolRef createTriRef(size_t vertIdx[3]);
		static TopolRef createPolylineRef(size_t meshIdx, size_t polylineNumber, size_t polylineIndex);
		static TopolRef createGridEdgeMidPtRef(const GridEdge& edge);
		static TopolRef createGridFaceCentroidRef(const GridFace& face);

		TopolRef();
		TopolRef(const TopolRef& src) = default;
		bool verify(const GridBase& grid) const;

		void save(std::ostream& out) const;
		bool readVersion1(std::istream& in);

		ClampType getClampType() const;
		bool matches(int mask) const;
		const Vector3d& getVector() const;

		size_t getMeshIdx() const;
		size_t getPolylineNumber() const;
		size_t getPolylineIndex() const;
		void setPolylineIndex(size_t idx);

		size_t getCellIdx() const;
		GridEdge getEdge() const;
		FaceNumber getFaceNumber() const;

		void getTriVertIndices(size_t indices[3]) const;

	private:
		ClampType _clampType = CLAMP_NONE;
		size_t _indices[3];
		Vector3d _v;
	};

	inline TopolRef TopolRef::createFixed() {
		TopolRef result;
		result._clampType = CLAMP_FIXED;
		return result;
	}

	inline TopolRef TopolRef::createNone() {
		TopolRef result;
		result._clampType = CLAMP_NONE;
		return result;
	}

	inline TopolRef TopolRef::createPerpendicular(const Vector3d& v) {
		TopolRef result;
		result._clampType = CLAMP_PERPENDICULAR;
		result._v = v;
		return result;
	}

	inline TopolRef TopolRef::createParallel(const Vector3d& v) {
		TopolRef result;
		result._clampType = CLAMP_PARALLEL;
		result._v = v;
		return result;
	}

	inline TopolRef TopolRef::createVert(size_t meshIdx, size_t vertIdx) {
		TopolRef result;
		result._clampType = CLAMP_VERT;
		result._indices[0] = meshIdx;
		result._indices[1] = vertIdx;
		return result;
	}

	inline TopolRef TopolRef::createTriRef(size_t vertIdx[3]) {
		TopolRef result;
		result._clampType = CLAMP_GRID_TRI_PLANE;
		result._indices[0] = vertIdx[0];
		result._indices[1] = vertIdx[1];
		result._indices[2] = vertIdx[2];

		return result;
	}

	inline TopolRef TopolRef::createPolylineRef(size_t meshIdx, size_t polylineNumber, size_t polylineIndex) {
		TopolRef result;
		result._clampType = CLAMP_EDGE;
		result._indices[0] = meshIdx;
		result._indices[1] = polylineNumber;
		result._indices[2] = polylineIndex;

		return result;
	}

	inline TopolRef TopolRef::createGridEdgeMidPtRef(const GridEdge& edge) {
		TopolRef result;
		if (edge.getVert(0) == stm1 || edge.getVert(1) == stm1) {
			throw "initialized edge mid pt ref with bad edge";
		}
		result._clampType = CLAMP_CELL_EDGE_CENTER;
		result._indices[0] = edge.getVert(0);
		result._indices[1] = edge.getVert(1);

		return result;
	}

	inline TopolRef TopolRef::createGridFaceCentroidRef(const GridFace& faceRef) {
		TopolRef result;
		result._clampType = CLAMP_CELL_FACE_CENTER;
		result._indices[0] = faceRef.getCellIdx();
		result._indices[1] = faceRef.getFaceNumber();

		return result;
	}

	inline TopolRef::TopolRef()
		: _clampType(CLAMP_NONE)
		, _v(DBL_MAX, DBL_MAX, DBL_MAX)
	{
		_indices[0] = _indices[1] = _indices[2] = stm1;
	}

	inline ClampType TopolRef::getClampType() const {
		return _clampType;
	}

	inline bool TopolRef::matches(int mask) const {
		if (_clampType == CLAMP_UNKNOWN)
			throw "Unknown clamp type";
		return (_clampType & mask) != 0;
	}

	inline const Vector3d& TopolRef::getVector() const {
		return _v;
	}

	inline size_t TopolRef::getMeshIdx() const {
		if (!matches(CLAMP_VERT | CLAMP_EDGE | CLAMP_TRI))
			throw "Wrong ClampType for this TopolRef";
		
		return _indices[0];
	}

	inline size_t TopolRef::getPolylineNumber() const {
		if (_clampType != CLAMP_EDGE)
			throw "Wrong ClampType for this TopolRef";
		return _indices[1];
	}

	inline size_t TopolRef::getPolylineIndex() const {
		if (_clampType != CLAMP_EDGE)
			throw "Wrong ClampType for this TopolRef";
		return _indices[2];
	}

	inline void TopolRef::setPolylineIndex(size_t idx) {
		if (_clampType != CLAMP_EDGE)
			throw "Wrong ClampType for this TopolRef";
		_indices[2] = idx;
	}

	inline void TopolRef::getTriVertIndices(size_t indices[3]) const {
		if (_clampType != CLAMP_GRID_TRI_PLANE)
			throw "Wrong ClampType for this TopolRef";
		for (int i = 0; i < 3; i++)
			indices[i] = _indices[i];
	}

	inline size_t TopolRef::getCellIdx() const {
		if (!matches(CLAMP_CELL_EDGE_CENTER | CLAMP_CELL_FACE_CENTER))
			throw "Wrong ClampType for this TopolRef";
		return _indices[0];
	}

	inline GridEdge TopolRef::getEdge() const {
		if (_clampType != CLAMP_CELL_EDGE_CENTER)
			throw "Wrong ClampType for this TopolRef";
		return GridEdge (_indices[0], _indices[1]);
	}

	inline FaceNumber TopolRef::getFaceNumber() const {
		if (_clampType != CLAMP_CELL_FACE_CENTER)
			throw "Wrong ClampType for this TopolRef";
		return (FaceNumber) _indices[1];
	}

}
