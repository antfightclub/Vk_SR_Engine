#pragma once

#include <vk_types.h>

//> DescriptorLayoutBuilder
struct DescriptorLayoutBuilder {
	std::vector<vk::DescriptorSetLayoutBinding> bindings;

	void add_binding(uint32_t binding, vk::DescriptorType type);
	void clear();
	vk::DescriptorSetLayout build(vk::Device device, vk::ShaderStageFlags shaderStages, void* pNext = nullptr, vk::DescriptorSetLayoutCreateFlags flags);
};
//< DescriptorLayoutBuilder

//> DescriptorAllocator
struct DescriptorAllocator {
	struct PoolSizeRatio {
		vk::DescriptorType type;
		float ratio;
	};

	vk::DescriptorPool pool;

	void init_pool(vk::Device device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
	void clear_descriptors(vk::Device device);
	void destroy_poool(vk::Device device);

	vk::DescriptorSet allocate(vk::Device device, vk::DescriptorSetLayout layout);
};
//< DescriptorAllocator

//> DescriptorAllocatorGrowable
struct DescriptorAllocatorGrowable {
public:
	struct PoolSizeRatio {
		vk::DescriptorType type;
		float ratio;
	};
	
	void init(vk::Device device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
	void clear_pools(vk::Device device);
	void destroy_pools(vk::Device device);
	
	vk::DescriptorSet allocate(vk::Device device, vk::DescriptorSetLayout layout, void* pNext = nullptr);
private:
	vk::DescriptorPool get_pool(vk::Device device);
	vk::DescriptorPool create_pool(vk::Device device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

	std::vector<PoolSizeRatio> ratios;
	std::vector<vk::DescriptorPool> fullPools;
	std::vector<vk::DescriptorPool> readyPools;
	uint32_t setsPerPool;
}; 
//< DescriptorAllocatorGrowable

//> DescriptorWriter
struct DescriptorWriter {
	std::deque<vk::DescriptorImageInfo> imageInfos;
	std::deque<vk::DescriptorBufferInfo> bufferInfos;
	std::vector<vk::WriteDescriptorSet> writes;

	void write_image(int binding, vk::ImageView image, vk::Sampler sampler, vk::ImageLayout layout, vk::DescriptorType type);
	void write_buffer(int binding, vk::Buffer buffer, size_t size, size_t offset, vk::DescriptorType type);

	void clear();
	void update_set(vk::Device device, vk::DescriptorSet set);
};
//< DescriptorWriter