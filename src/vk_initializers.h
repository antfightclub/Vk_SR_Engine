//vk_initializers.h
#pragma once

#include <vk_types.h>

namespace vkinit 
{
	//> init_cmds
	vk::CommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags = 0);
	vk::CommandBufferAllocateInfo command_buffer_allocate_info(vk::CommandPool pool, uint32_t count = 1);
	//< init_cmds

	//> init_images
	vk::ImageCreateInfo image_create_info(vk::Format format, vk::ImageUsageFlags usageFlags, vk::Extent3D extent);
	vk::ImageViewCreateInfo imageview_create_info(vk::Format format, vk::Image image, vk::ImageAspectFlags aspectFlags);
	//< init_images
}