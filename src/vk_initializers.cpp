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

vk::CommandBufferBeginInfo vkinit::command_buffer_begin_info(vk::CommandBufferUsageFlags flags) {
	vk::CommandBufferBeginInfo info = {};
	info.pInheritanceInfo = nullptr;
	info.flags = flags;
	return info;
}

vk::CommandBufferSubmitInfo vkinit::command_buffer_submit_info(vk::CommandBuffer cmd) {
	vk::CommandBufferSubmitInfo info = {};
	info.commandBuffer = cmd;
	info.deviceMask = 0;
	return info;
}

vk::SubmitInfo2 vkinit::submit_info(vk::CommandBufferSubmitInfo* cmd, vk::SemaphoreSubmitInfo* signalSemaphoreInfo, vk::SemaphoreSubmitInfo* waitSemaphoreInfo) {
	vk::SubmitInfo2 info = {};

	info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
	info.pWaitSemaphoreInfos = waitSemaphoreInfo;

	info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
	info.pSignalSemaphoreInfos = signalSemaphoreInfo;

	info.commandBufferInfoCount = 1;
	info.pCommandBufferInfos = cmd;

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

vk::SemaphoreCreateInfo vkinit::semaphore_create_info() {
	vk::SemaphoreCreateInfo info = {};
	
	return info;
}
//< init_sync_structures