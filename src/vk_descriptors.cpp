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
void DescriptorAllocator::init_pool(vk::Device device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios) {
	std::vector<vk::DescriptorPoolSize> poolSizes;
	for (PoolSizeRatio ratio : poolRatios)	{
		poolSizes.push_back(vk::DescriptorPoolSize{ 
			ratio.type, 
			uint32_t(ratio.ratio * maxSets)
			});
	}

	vk::DescriptorPoolCreateInfo pool_info = {};
	pool_info.maxSets = maxSets;
	pool_info.poolSizeCount = (uint32_t)poolSizes.size();
	pool_info.pPoolSizes = poolSizes.data();

	device.createDescriptorPool(&pool_info, nullptr, &pool);
}

void DescriptorAllocator::clear_descriptors(vk::Device device) {
	device.resetDescriptorPool(pool);
}

void DescriptorAllocator::destroy_poool(vk::Device device) {
	device.destroyDescriptorPool(pool, nullptr);
}

vk::DescriptorSet DescriptorAllocator::allocate(vk::Device device, vk::DescriptorSetLayout layout) {
	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	vk::DescriptorSet ds;
	VK_CHECK(device.allocateDescriptorSets(&allocInfo, &ds));

	return ds;
}
//< DescriptorAllocator

//> DescriptorAllocatorGrowable

//< DescriptorAllocatorGrowable

//> DescriptorWriter

//< DescriptorWriter