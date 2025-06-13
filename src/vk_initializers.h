//vk_initializers.h
#pragma once

#include <vk_types.h>

namespace vkinit {
	//> images
	vk::ImageCreateInfo image_create_info(vk::Format format, vk::ImageUsageFlags usageFlags, vk::Extent3D extent);
	vk::ImageViewCreateInfo imageview_create_info(vk::Format format, vk::Image image, vk::ImageAspectFlags aspectFlags);
	//< images
}