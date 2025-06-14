//vk_initializers.cpp
#include "vk_initializers.h"

//> init_cmds
vk::CommandPoolCreateInfo vkinit::command_pool_create_info(uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags) {
	vk::CommandPoolCreateInfo info = {};
	info.queueFamilyIndex = queueFamilyIndex;
	info.flags = flags;
	return info;
}

vk::CommandBufferAllocateInfo vkinit::command_buffer_allocate_info(vk::CommandPool pool, uint32_t count) {
	vk::CommandBufferAllocateInfo info = {};
	info.commandPool = pool;
	info.commandBufferCount = count;
	info.level = vk::CommandBufferLevel::ePrimary;
	return info;
}
//< init_cmds

//> init_images
vk::ImageCreateInfo vkinit::image_create_info(vk::Format format, vk::ImageUsageFlags usageFlags, vk::Extent3D extent) {
	vk::ImageCreateInfo info = {};

	info.imageType = vk::ImageType::e2D;

	info.format = format;
	info.extent = extent;
	
	info.mipLevels = 1;
	info.arrayLayers = 1;

	// Change this for MSAA - I'm not using it yet.
	info.samples = vk::SampleCountFlagBits::e1;

	// Optimal tiling, which means the image is stored in the best GPU format (i.e. we don't really get a say in the matter)
	info.tiling = vk::ImageTiling::eOptimal;
	
	info.usage = usageFlags;
	return info;
}

vk::ImageViewCreateInfo vkinit::imageview_create_info(vk::Format format, vk::Image image, vk::ImageAspectFlags aspectFlags) {
	// Build an image-view for the given image to use for rendering

	vk::ImageViewCreateInfo info = {};

	info.viewType = vk::ImageViewType::e2D;
	info.image = image;
	info.format = format;
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = 1;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = 1;
	info.subresourceRange.aspectMask = aspectFlags;

	return info;
}
//< init_images

//> init_sync_structures
vk::FenceCreateInfo vkinit::fence_create_info(vk::FenceCreateFlags flags) {
	vk::FenceCreateInfo info = {};
	info.flags = flags;

	return info;
}

vk::SemaphoreCreateInfo vkinit::semaphore_create_info(vk::SemaphoreCreateFlags flags) {
	vk::SemaphoreCreateInfo info = {};
	info.flags = flags;
	
	return info;
}
//< init_sync_structures