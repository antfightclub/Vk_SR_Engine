#include "vk_descriptors.h"

//> DescriptorLayoutBuilder
void DescriptorLayoutBuilder::add_binding(uint32_t binding, vk::DescriptorType type) {
	vk::DescriptorSetLayoutBinding newbind = {};
	newbind.binding = binding;
	newbind.descriptorCount = 1;
	newbind.descriptorType = type;

	bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::clear() {
	bindings.clear();
}

vk::DescriptorSetLayout DescriptorLayoutBuilder::build(vk::Device device, vk::ShaderStageFlags shaderStages, void* pNext = nullptr, vk::DescriptorSetLayoutCreateFlags flags) {
	for (auto& b : bindings) {
		b.stageFlags |= shaderStages;
	}

	vk::DescriptorSetLayoutCreateInfo info = {};
	info.pNext = pNext;

	info.pBindings = bindings.data();
	info.bindingCount = (uint32_t)bindings.size();
	info.flags = flags;

	vk::DescriptorSetLayout set;
	VK_CHECK(device.createDescriptorSetLayout(&info, nullptr, &set));
}
//< DescriptorLayoutBuilder

//> DescriptorAllocator

//< DescriptorAllocator

//> DescriptorAllocatorGrowable

//< DescriptorAllocatorGrowable

//> DescriptorWriter

//< DescriptorWriter