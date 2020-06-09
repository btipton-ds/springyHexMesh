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

#include <array>
#include <map>
#include <algorithm>

#include <tm_math.h>
#include <vk_buffer.h>
#include <hm_ui_root.h>
#include <hm_ui_gridTriNode.h>

using namespace std;
using namespace HexahedralMesher;

UI::GridTriNode::GridTriNode(Root* root, VK::Pipeline3DPtr& ownerPipeline)
	: PipelineSceneNode3D(ownerPipeline)
	, _root(root)
	, _indices(ownerPipeline->getApp()->getDeviceContext())
{}

UI::GridTriNode::~GridTriNode() {
}

void UI::GridTriNode::setFaceDrawList(const vector<uint32_t>& indices) {
	_numIndices = (uint32_t)indices.size();
	_indices.create(indices, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void UI::GridTriNode::addCommands(VkCommandBuffer cmdBuff, VkPipelineLayout pipelineLayout, size_t swapChainIndex) const {
	VkBuffer vertexBuffers[] = { *_root->getPosNormVertBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmdBuff, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(cmdBuff, _indices, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &_descriptorSets[swapChainIndex], 0, nullptr);

	vkCmdDrawIndexed(cmdBuff, _numIndices, 1, 0, 0, 0);
}

void UI::GridTriNode::buildImageInfoList(std::vector<VkDescriptorImageInfo>& imageInfoList) const {

}

typename UI::GridTriNode::BoundingBox UI::GridTriNode::getBounds() const {
	return this->_root->getBounds();
}

void UI::GridTriNode::createDescriptorPool() {
	auto app = _ownerPipeline->getApp();
	const auto& swap = app->getSwapChain();
	auto devCon = app->getDeviceContext()->device_;

	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(swap._vkImages.size());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(swap._vkImages.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swap._vkImages.size());

	if (vkCreateDescriptorPool(devCon, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void UI::GridTriNode::createDescriptorSets() {
	auto app = _ownerPipeline->getApp();
	auto dc = app->getDeviceContext()->device_;

	const auto& swap = app->getSwapChain();
	size_t swapChainSize = (uint32_t)swap._vkImages.size();

	std::vector<VkDescriptorSetLayout> layouts(swapChainSize, _ownerPipeline->getDescriptorSetLayout());
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainSize);
	allocInfo.pSetLayouts = layouts.data();

	_descriptorSets.resize(swapChainSize);

	if (vkAllocateDescriptorSets(dc, &allocInfo, _descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < swapChainSize; i++) {
		VkDescriptorBufferInfo bufferInfo = {};

		bufferInfo.buffer = _uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = _descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(dc, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void UI::GridTriNode::createUniformBuffers() {
	auto app = _ownerPipeline->getApp();
	size_t bufferSize = sizeof(UniformBufferObject);
	const auto& swap = app->getSwapChain();
	size_t swapChainSize = (uint32_t)swap._vkImages.size();

	_uniformBuffers.clear();
	_uniformBuffers.reserve(swapChainSize);

	for (size_t i = 0; i < swapChainSize; i++) {
		_uniformBuffers.push_back(VK::Buffer(app->getDeviceContext()));
		_uniformBuffers.back().create(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

}
