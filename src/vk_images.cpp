#include <vk_images.h>
#include <vk_initializers.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void vkutil::transition_image(vk::CommandBuffer cmd, vk::Image image, vk::ImageLayout currentLayout, vk::ImageLayout newLayout) {
	vk::ImageMemoryBarrier2 imageBarrier = {};

	imageBarrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
	imageBarrier.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite;
	imageBarrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
	imageBarrier.dstAccessMask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead;

	imageBarrier.oldLayout = currentLayout;
	imageBarrier.newLayout = newLayout;

	vk::ImageAspectFlags aspectMask = (newLayout == vk::ImageLayout::eDepthAttachmentOptimal) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
	
	imageBarrier.subresourceRange = vkinit::image_subresource_range(aspectMask);
	imageBarrier.image = image;

	vk::DependencyInfo depInfo = {};
	
	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers = &imageBarrier;

	cmd.pipelineBarrier2(&depInfo);
}

void vkutil::copy_image_to_image(vk::CommandBuffer cmd, vk::Image source, vk::Image destination, vk::Extent2D srcSize, vk::Extent2D dstSize) {
	vk::ImageBlit2 blitRegion = {};

	blitRegion.srcOffsets[1].x = srcSize.width;
	blitRegion.srcOffsets[1].y = srcSize.height;
	blitRegion.srcOffsets[1].z = 1;

	blitRegion.dstOffsets[1].x = dstSize.width;
	blitRegion.dstOffsets[1].y = dstSize.height;
	blitRegion.dstOffsets[1].z = 1;

	blitRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;	
	
	blitRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	vk::BlitImageInfo2 blitInfo = {};

	blitInfo.dstImage = destination;
	blitInfo.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
	
	blitInfo.srcImage = source;
	blitInfo.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;

	blitInfo.filter = vk::Filter::eLinear;

	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

	cmd.blitImage2(&blitInfo);
}

void vkutil::generate_mipmaps(vk::CommandBuffer cmd, vk::Image image, vk::Extent2D imageSize) {
	int mipLevels = int(std::floor(std::log2(std::max(imageSize.width, imageSize.height)))) + 1;
	for (int mip = 0; mip < mipLevels; mip++) {
		// Shrink size in half
		vk::Extent2D halfSize = imageSize;
		halfSize.width /= 2;
		halfSize.height /= 2;

		vk::ImageMemoryBarrier2 imageBarrier = {};

		imageBarrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
		imageBarrier.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite;
		imageBarrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
		imageBarrier.dstAccessMask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead;

		imageBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		imageBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;

		vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor;
		
		imageBarrier.subresourceRange = vkinit::image_subresource_range(aspectMask);
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.baseMipLevel = mip;
		imageBarrier.image = image;

		vk::DependencyInfo depInfo = {};
		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &imageBarrier;

		cmd.pipelineBarrier2(&depInfo);
		
		if (mip < mipLevels - 1) {
			vk::ImageBlit2 blitRegion = {};
			
			blitRegion.srcOffsets[1].x = imageSize.width;
			blitRegion.srcOffsets[1].y = imageSize.height;
			blitRegion.srcOffsets[1].z = 1;

			blitRegion.dstOffsets[1].x = halfSize.width;
			blitRegion.dstOffsets[1].y = halfSize.height;
			blitRegion.dstOffsets[1].z = 1;

			blitRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			blitRegion.srcSubresource.baseArrayLayer = 0;
			blitRegion.srcSubresource.layerCount = 1;
			blitRegion.srcSubresource.mipLevel = mip;

			blitRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			blitRegion.dstSubresource.baseArrayLayer = 0;
			blitRegion.dstSubresource.layerCount = 1;
			blitRegion.dstSubresource.mipLevel = mip + 1;

			vk::BlitImageInfo2 blitInfo = {};

			blitInfo.dstImage = image;
			blitInfo.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
			
			blitInfo.srcImage = image;
			blitInfo.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;

			blitInfo.filter = vk::Filter::eLinear;
			blitInfo.regionCount = 1;
			blitInfo.pRegions = &blitRegion;

			cmd.blitImage2(&blitInfo);

			imageSize = halfSize;
		}
	}
	
	// Transition all mip levels into the final read only layout
	transition_image(cmd, image, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
}