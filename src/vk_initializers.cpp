//vk_initializers.cpp
#include "vk_initializers.h"

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
//< images