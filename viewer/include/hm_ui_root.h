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

#include <vk_defines.h>

#include <memory>
#include <vector>

#include <hm_forwardDeclarations.h>
#include <hm_ui_gridIndexNode.h>

#include <meshProcessor.h>
#include <vk_pipelineSceneNode3D.h>
#include <vk_pipeline3D.h>
#include <vk_app.h>

namespace HexahedralMesher {

	namespace UI {

		class GridIndexNode;
		using GridIndexNodePtr = std::shared_ptr<GridIndexNode>;

		class Root;
		using RootPtr = std::shared_ptr<Root>;

		class Root : public CMesher::Reporter, public VK::VulkanApp::Updater {
		public:
			using BoundingBox = VK::Pipeline3D::BoundingBox;

			Root(const CMesherPtr& mesher);
			virtual ~Root();

			// GLFW events only work from the main thread. So the GUI needs to be the main thread.
			bool run();

			VK::VulkanAppPtr getApp() const;
			const VK::Pipeline3DPtr& getPipelineTriShaded() const;
			const VK::Pipeline3DPtr& getPipelineTriWire() const;
			const VK::Pipeline3DPtr& getPipelineLines() const;

			void report(const CMesher& mesher, const std::string& key) override;
			void reportModelAdded(const CMesher& mesher, const CModelPtr& model) override;

			void updateVkApp() override;

			bool isRunning() const override;

			const VK::BufferPtr& getPosNormVertBuffer() const;

			inline const BoundingBox& getBounds() const;

		private:
			enum State {
				STATE_IDLE = 0,
				STATE_PENDING = 1,
				STATE_WORKING = 2,
				STATE_TOPOL_CHANGED = 4,
				STATE_VERTS_UDATED = 8,
			};

			void buildUi(const VK::UI::WindowPtr& win);
			void addViewButtons(const VK::UI::WindowPtr& win);
			void buildBuffers();
			void buildPosNormBuffer(bool buildTopology);
			void updateVerts();
			void addGridFaces();
			void waitTillClear(size_t bits) const;
			void setState(size_t bits);

			bool _isRunning = true;

			struct ModelRec {
				CModelPtr _model;
				VK::SceneNode3DPtr _sceneNodeShaded, _scenNodeWf, _sceneNodeEdges;
			};
			std::vector<ModelRec> _models;

			CMesherPtr _mesher;

			VK::VulkanAppPtr _app;
			VK::Pipeline3DPtr _plShaded, _plTriWire, _plEdges, _plFaceBounds;
			GridIndexNodePtr _allGridFacesShaded, _allGridEdges, _allGridFaceBounds;

			BoundingBox _bbox;
			size_t _updateBits = STATE_IDLE;
			std::vector<std::array<uint32_t, 3>> _tris;
			std::map<SearchableFace, std::array<size_t, 2>> _faceToTriMap;
			VK::BufferPtr _posNormVertBuffer; // Vertex buffer with normals for each tri direction
		};


		inline VK::VulkanAppPtr Root::getApp() const {
			return _app;
		}

		inline const VK::Pipeline3DPtr& Root::getPipelineTriShaded() const {
			return _plShaded;
		}

		inline const VK::Pipeline3DPtr& Root::getPipelineTriWire() const {
			return _plTriWire;
		}

		inline const VK::Pipeline3DPtr& Root::getPipelineLines() const {
			return _plEdges;
		}

		inline const VK::BufferPtr& Root::getPosNormVertBuffer() const {
			return _posNormVertBuffer;
		}

		inline const Root::BoundingBox& Root::getBounds() const {
			return _bbox;
		}

	}
}