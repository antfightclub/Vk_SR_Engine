//vk_initializers.cpp
#include "vk_initializers.h"

//> cmds
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
//< cmds

//> images
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

vk::ImageSubresourceRange vkinit::image_subresource_range(vk::ImageAspectFlags aspectMask) {
	vk::ImageSubresourceRange subImage = {};
	subImage.aspectMask = aspectMask;
	subImage.baseMipLevel = 0;
	subImage.levelCount = vk::RemainingMipLevels;
	subImage.baseArrayLayer = 0;
	subImage.layerCount = vk::RemainingArrayLayers;

	return subImage;
}
//< images

//> sync_structures
vk::FenceCreateInfo vkinit::fence_create_info(vk::FenceCreateFlags flags) {
	vk::FenceCreateInfo info = {};
	info.flags = flags;

	return info;
}

vk::SemaphoreCreateInfo vkinit::semaphore_create_info() {
	vk::SemaphoreCreateInfo info = {};
	
	return info;
}

vk::SemaphoreSubmitInfo  vkinit::semaphore_submit_info(vk::PipelineStageFlagBits2 stageMask, vk::Semaphore semaphore) {
	vk::SemaphoreSubmitInfo submitInfo = {};

	submitInfo.semaphore = semaphore;
	submitInfo.stageMask = stageMask;
	submitInfo.deviceIndex = 0;
	submitInfo.value = 1;

	return submitInfo;
}
//< sync_structures

//> present
vk::PresentInfoKHR vkinit::present_info() {
	vk::PresentInfoKHR info = {};

	info.swapchainCount = 0;
	info.pSwapchains = nullptr;
	info.pWaitSemaphores = nullptr;
	info.waitSemaphoreCount = 0;
	info.pImageIndices = nullptr;

	return info;
}
//< present

//> pipeline
vk::PipelineLayoutCreateInfo vkinit::pipeline_layout_create_info() {
	vk::PipelineLayoutCreateInfo info = {};

	// Empty defaults
	info.flags = {};
	info.setLayoutCount = 0;
	info.pSetLayouts = nullptr;
	info.pushConstantRangeCount = 0;
	info.pPushConstantRanges = nullptr;
	return info;
}

vk::PipelineShaderStageCreateInfo vkinit::pipeline_shader_stage_create_info(vk::ShaderStageFlagBits stage, vk::ShaderModule shaderModule, const char* entry) {
	vk::PipelineShaderStageCreateInfo info = {};

	// Shader stage
	info.stage = stage;

	// Module containing the code for this shader stage
	info.module = shaderModule;

	// The entry point of the shader
	info.pName = entry;
	return info;
}
//< pipeline

//> attachments
vk::RenderingAttachmentInfo vkinit::attachment_info(vk::ImageView view, vk::ClearValue* clear, vk::ImageLayout layout) {
	vk::RenderingAttachmentInfo colorAttachment = {};
	colorAttachment.imageView = view;
	colorAttachment.imageLayout = layout;
	colorAttachment.loadOp = clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	if (clear) {
		colorAttachment.clearValue = *clear;
	}
	return colorAttachment;
}

vk::RenderingAttachmentInfo vkinit::depth_attachment_info(vk::ImageView view, vk::ImageLayout layout) {
	vk::RenderingAttachmentInfo depthAttachment = {};
	depthAttachment.imageView = view;
	depthAttachment.imageLayout = layout;
	depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	depthAttachment.clearValue.depthStencil.depth = 0.f;
	return depthAttachment;
}
//< attachments

//> render_info
vk::RenderingInfo vkinit::rendering_info(vk::Extent2D renderExtent, vk::RenderingAttachmentInfo* colorAttachment, vk::RenderingAttachmentInfo* depthAttachment) {
	vk::RenderingInfo renderInfo = {};

	renderInfo.renderArea = vk::Rect2D{ vk::Offset2D{0, 0 }, renderExtent };
	renderInfo.layerCount = 1;
	renderInfo.colorAttachmentCount = 1;
	renderInfo.pColorAttachments = colorAttachment;
	renderInfo.pDepthAttachment = depthAttachment;
	renderInfo.pStencilAttachment = nullptr;
	return renderInfo;
}
//< render_info