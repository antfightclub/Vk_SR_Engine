//vk_initializers.h
#pragma once

#include <vk_types.h>

namespace vkinit 
{
	//> cmds
	vk::CommandPoolCreateInfo     command_pool_create_info(uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags);
	vk::CommandBufferAllocateInfo command_buffer_allocate_info(vk::CommandPool pool, uint32_t count = 1);
	vk::CommandBufferBeginInfo    command_buffer_begin_info(vk::CommandBufferUsageFlags flags);
	vk::CommandBufferSubmitInfo   command_buffer_submit_info(vk::CommandBuffer cmd);
	vk::SubmitInfo2				  submit_info(vk::CommandBufferSubmitInfo* cmd, vk::SemaphoreSubmitInfo* signalSemaphoreInfo, vk::SemaphoreSubmitInfo* waitSemaphoreInfo);
	//< cmds

	//> images
	vk::ImageCreateInfo       image_create_info(vk::Format format, vk::ImageUsageFlags usageFlags, vk::Extent3D extent);
	vk::ImageViewCreateInfo   imageview_create_info(vk::Format format, vk::Image image, vk::ImageAspectFlags aspectFlags);
	vk::ImageSubresourceRange image_subresource_range(vk::ImageAspectFlags aspectMask);
	//< images

	//> sync_structures
	vk::FenceCreateInfo     fence_create_info(vk::FenceCreateFlags flags);
	vk::SemaphoreCreateInfo semaphore_create_info(/*vk::SemaphoreCreateFlags flags*/);
	//< sync_structures
}