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

#include <tm_edge.h>
#include <meshProcessor.h>
#include <vk_pipelineSceneNode3D.h>
#include <vk_app.h>

namespace HexahedralMesher {

	namespace UI {

		class ModelEdges;
		using ModelEdgesPtr = std::shared_ptr<ModelEdges>;

		class ModelEdges : public VK::PipelineSceneNode3D {
		public:
			using BoundingBox = CBoundingBox3D<float>;
			using VertexType = VK::Pipeline3D::VertexType;

			static inline ModelEdgesPtr create(const VK::PipelineBasePtr& ownerPipeline, const CModelPtr& model) {
				return std::shared_ptr<ModelEdges>(new ModelEdges(ownerPipeline, model));
			}

			void addCommands(VkCommandBuffer cmdBuff, VkPipelineLayout pipelineLayout, size_t swapChainIndex) const override;
			void buildImageInfoList(std::vector<VkDescriptorImageInfo>& imageInfoList) const override;
			BoundingBox getBounds() const override;

			inline const VK::Buffer& getVertexBuffer() const {
				return _vertexBuffer;
			}

			inline const VK::Buffer& getIndexBuffer() const {
				return _indexBuffer;
			}

			void createDescriptorPool() override;
			void createDescriptorSets() override;
			void createUniformBuffers() override;

		protected:
			ModelEdges(const VK::PipelineBasePtr& ownerPipeline);
			ModelEdges(const VK::PipelineBasePtr& ownerPipeline, const CModelPtr& model);

			void loadModel(const CModelPtr& model);

			BoundingBox _bounds;
			uint32_t _numIndices = 0;
			VK::Buffer _vertexBuffer, _indexBuffer;
		};

	}
}
