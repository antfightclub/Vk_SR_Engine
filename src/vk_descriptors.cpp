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
void DescriptorAllocatorGrowable::init(vk::Device device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios) {
	ratios.clear();

	for (auto r : poolRatios) {
		ratios.push_back(r);
	}

	vk::DescriptorPool newPool = create_pool(device, initialSets, poolRatios);

	setsPerPool = initialSets * 1.5; // Grow the pool for next allocation

	readyPools.push_back(newPool);
}

void DescriptorAllocatorGrowable::clear_pools(vk::Device device) {
	for (auto p : readyPools) {
		device.resetDescriptorPool(p);
	}
	for (auto p : fullPools) {
		device.resetDescriptorPool(p);
		readyPools.push_back(p);
	}
	fullPools.clear();
}

void DescriptorAllocatorGrowable::destroy_pools(vk::Device device) {
	for (auto p : readyPools) {
		device.destroyDescriptorPool(p, nullptr);
	}
	readyPools.clear();
	for (auto p : fullPools) {
		device.destroyDescriptorPool(p, nullptr);
	}
	fullPools.clear();
}

vk::DescriptorSet  DescriptorAllocatorGrowable::allocate(vk::Device device, vk::DescriptorSetLayout layout, void* pNext = nullptr) {
	vk::DescriptorPool poolToUse = get_pool(device);

	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.pNext = pNext;
	allocInfo.descriptorPool = poolToUse;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	vk::DescriptorSet ds;

	vk::Result result = device.allocateDescriptorSets(&allocInfo, &ds);

	// Allocation failed, try again
	if (result == vk::Result::eErrorOutOfPoolMemory || result == vk::Result::eErrorFragmentedPool) {
		fullPools.push_back(poolToUse);
		poolToUse = get_pool(device);

		allocInfo.descriptorPool = poolToUse;
		VK_CHECK(device.allocateDescriptorSets(&allocInfo, &ds));
	}

	readyPools.push_back(poolToUse);
	return ds;
}

vk::DescriptorPool DescriptorAllocatorGrowable::get_pool(vk::Device device) {
	vk::DescriptorPool newPool;
	if (readyPools.size() != 0) {
		newPool = readyPools.back();
		readyPools.pop_back();
	}
	else {
		// need to create a new pool
		newPool = create_pool(device, setsPerPool, ratios);

		setsPerPool = setsPerPool * 1.5;
		if (setsPerPool > 4092) {
			setsPerPool = 4092;
		}
	}

	return newPool;
}

vk::DescriptorPool DescriptorAllocatorGrowable::create_pool(vk::Device device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios) {
	std::vector<vk::DescriptorPoolSize> poolSizes;
	for (PoolSizeRatio ratio : poolRatios) {
		poolSizes.push_back(vk::DescriptorPoolSize{
			ratio.type, uint32_t(ratio.ratio * setCount) 
			});
	}

	vk::DescriptorPoolCreateInfo pool_info = {};
	pool_info.maxSets = setCount;
	pool_info.poolSizeCount = (uint32_t)poolSizes.size();
	pool_info.pPoolSizes = poolSizes.data();

	vk::DescriptorPool newPool;
	VK_CHECK(device.createDescriptorPool(&pool_info, nullptr, &newPool));

	return newPool;
}
//< DescriptorAllocatorGrowable

//> DescriptorWriter

//< DescriptorWriter