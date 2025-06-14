//vk_engine.cpp
#include "vk_engine.h"

// SDL3 for windowing
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vk_types.h>
#include <vk_initializers.h>

// Simplify Vulkan initialization
#include "VkBootstrap.h"

// Vulkan Memory Allocator to help with memory allocation
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h" // For some reason I *need* to have this here in this order, otherwise linker errors!
#include "vk_mem_alloc.hpp"

#include <chrono>
#include <thread>

// Could use a preprocessor directive to define as true on debug build
// and false on release build
constexpr bool bUseValidationLayers = true;

VkSREngine* loadedEngine = nullptr;

VkSREngine& VkSREngine::Get() { return *loadedEngine; }

void VkSREngine::init() 
{
	// Allow only one engine initialization at one time
	assert(loadedEngine == nullptr);
	loadedEngine = this;

	// Initialize SDL and create a window with it
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

	_window = SDL_CreateWindow(
		"Vk SR Engine",
		_windowExtent.width,
		_windowExtent.height,
		window_flags
	);


	const SDL_DisplayMode* DM = SDL_GetCurrentDisplayMode(1);
	_largestExtent.setHeight((uint32_t)DM->h);
	_largestExtent.setWidth((uint32_t)DM->w);

	init_vulkan();
	
	init_swapchain();

	init_commands();

	init_sync_structures();

	_isInitialized = true;
}

void VkSREngine::init_vulkan() 
{
	// Using vk-bootstrap to initialize vulkan instance
	vkb::InstanceBuilder builder;

	// Make a vulkan instance with basic debug features
	auto inst_ret = builder.set_app_name("VkSREngine")
		.request_validation_layers(bUseValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3)
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	// Grab the instance and debug messenger callback
	_instance = vkb_inst.instance;
	_debug_messenger = vkb_inst.debug_messenger;

	SDL_Vulkan_CreateSurface(_window, _instance, VK_NULL_HANDLE, reinterpret_cast<VkSurfaceKHR*>(&_surface));

	// Choose features from different spec versions
	
	// Vulkan 1.3 features
	vk::PhysicalDeviceVulkan13Features features13 {};
	features13.dynamicRendering = true;
	features13.synchronization2 = true;

	// Vulkan 1.2 features
	vk::PhysicalDeviceVulkan12Features features12{};
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

	// Use VkBootstrap to select a GPU
	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features13)
		.set_required_features_12(features12)
		.set_surface(_surface)
		.select()
		.value();

	// Create the final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice and VkPhysicalDevice handles used in the rest of the vulkan application
	_device = vkbDevice.device;
	_chosenGPU = physicalDevice.physical_device;

	// Get the graphics queue handle
	_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	// Initialize Vulkan Memory Allocator
	vma::AllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = _chosenGPU;
	allocatorInfo.device = _device;
	allocatorInfo.instance = _instance;
	allocatorInfo.flags = vma::AllocatorCreateFlagBits::eBufferDeviceAddress;
	VK_CHECK(vma::createAllocator(&allocatorInfo, &_allocator));
}

//> swapchain
void VkSREngine::init_swapchain() {
	create_swapchain(_windowExtent.width, _windowExtent.height);
	//> drawimage
	// Use the largest
	vk::Extent3D drawImageExtent = {
		_largestExtent.width,
		_largestExtent.height,
		1
	};

	_drawImage.imageFormat = vk::Format::eR16G16B16A16Sfloat;
	_drawImage.imageExtent = drawImageExtent;

	vk::ImageUsageFlags drawImageUsages{
		  vk::ImageUsageFlagBits::eTransferSrc
		| vk::ImageUsageFlagBits::eStorage
		| vk::ImageUsageFlagBits::eColorAttachment
	};

	vk::ImageCreateInfo rimg_info = vkinit::image_create_info(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

	// For the draw image, allocate it from GPU-local memory
	vma::AllocationCreateInfo rimg_allocinfo = {};
	rimg_allocinfo.usage = vma::MemoryUsage::eGpuOnly;
	rimg_allocinfo.requiredFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

	// Allocate and create the draw image
	VK_CHECK(_allocator.createImage(&rimg_info, &rimg_allocinfo, &_drawImage.image, &_drawImage.allocation, nullptr ));

	vk::ImageViewCreateInfo rview_info = vkinit::imageview_create_info(_drawImage.imageFormat, _drawImage.image, vk::ImageAspectFlagBits::eColor);

	VK_CHECK(_device.createImageView(&rview_info, nullptr, &_drawImage.imageView));
	//< drawimage
	
	//> depthimage
	_depthImage.imageFormat = vk::Format::eD32Sfloat;
	_depthImage.imageExtent = drawImageExtent;

	vk::ImageUsageFlags depthImageUsages{
		vk::ImageUsageFlagBits::eDepthStencilAttachment
	};

	vk::ImageCreateInfo dimg_info = vkinit::image_create_info(_depthImage.imageFormat, depthImageUsages, drawImageExtent);

	// Allocate and create the draw image
	VK_CHECK(_allocator.createImage(&dimg_info, &rimg_allocinfo, &_depthImage.image, &_depthImage.allocation, nullptr));

	vk::ImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthImage.imageFormat, _depthImage.image, vk::ImageAspectFlagBits::eDepth);
	VK_CHECK(_device.createImageView(&dview_info, nullptr, &_depthImage.imageView));
	//< depthimage
	

	// Add to deletion queue
	_mainDeletionQueue.push_function([=]() {
		_device.destroyImageView(_drawImage.imageView, nullptr);
		_allocator.destroyImage(_drawImage.image, _drawImage.allocation);

		_device.destroyImageView(_depthImage.imageView, nullptr);
		_allocator.destroyImage(_depthImage.image, _depthImage.allocation);
		});
}

void VkSREngine::create_swapchain(uint32_t width, uint32_t height) {
	// Use vulkan bootstrap to create swapchain
	vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU, _device, _surface };

	_swapchainImageFormat = vk::Format::eB8G8R8A8Unorm;

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.set_desired_format(vk::SurfaceFormatKHR{ _swapchainImageFormat, vk::ColorSpaceKHR::eSrgbNonlinear })
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	_swapchainExtent = vkbSwapchain.extent;

	// Store swapchain and its related images
	_swapchain = vkbSwapchain.swapchain;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageviews;
	std::vector<vk::Image> vkimages;
	std::vector<vk::ImageView> vkimageviews;
	
	images = vkbSwapchain.get_images().value();
	imageviews = vkbSwapchain.get_image_views().value();
	
	// Unfortunately the swapchainbuilder doesn't play well with the Vulkan-Hpp headers...
	for (int i = 0; i < images.size(); i++) {
		vkimages.push_back((vk::Image)images[i]); // Will it seriously let me do this?
	}
	for (int i = 0; i < imageviews.size(); i++) {
		vkimageviews.push_back((vk::ImageView)imageviews[i]); // See previous question
	}
		
	_swapchainImages = vkimages;
	_swapchainImageViews = vkimageviews;

	// Set _swapchainImageCount to the amount of swapchain images - used to initialize 
	// the same amount of _readyForPresentSemaphores
	VK_CHECK(_device.getSwapchainImagesKHR(_swapchain, &_swapchainImageCount, nullptr));
}

void VkSREngine::destroy_swapchain() {
	_device.destroySwapchainKHR(_swapchain, nullptr);

	// Destroy swapchain resources
	for (int i = 0; i < _swapchainImageViews.size(); i++) {
		_device.destroyImageView(_swapchainImageViews[i], nullptr);
	}
}

void VkSREngine::resize_swapchain() {
	_device.waitIdle();

	destroy_swapchain();

	int w, h;
	SDL_GetWindowSize(_window, &w, &h);
	_windowExtent.width = w;
	_windowExtent.height = h;

	create_swapchain(_windowExtent.width, _windowExtent.height);

	resize_requested = false;
}
//< swapchain

//> init_commands
void VkSREngine::init_commands() {
	// Create a command pool for commands submitted to the graphics queue
	// The pool will allow for resetting of individual command buffers
	vk::CommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

	// Create per-frame command structures
	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(_device.createCommandPool(&commandPoolInfo, nullptr, &_frames[i]._commandPool));
		
		// Allocate the default command buffer that we will use for rendering
		vk::CommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool);

		VK_CHECK(_device.allocateCommandBuffers(&cmdAllocInfo, &_frames[i]._mainCommandBuffer));

		_mainDeletionQueue.push_function([=]() {
			_device.destroyCommandPool(_frames[i]._commandPool, nullptr);
			});
	}

	// Create immediate submit command structures
	VK_CHECK(_device.createCommandPool(&commandPoolInfo, nullptr, &_immCommandPool));

	vk::CommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_immCommandPool, 1);

	VK_CHECK(_device.allocateCommandBuffers(&cmdAllocInfo, &_immCommandBuffer));

	_mainDeletionQueue.push_function([=]() {
		_device.destroyCommandPool(_immCommandPool, nullptr);
		});
}
//< init_commands

//> init_sync_structures
void VkSREngine::init_sync_structures() {
	// Create the fence for immediate submit and add it to deletion queue
	vk::FenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(vk::FenceCreateFlagBits::eSignaled);
	VK_CHECK(_device.createFence(&fenceCreateInfo, nullptr, &_immFence));
	_mainDeletionQueue.push_function([=]() { _device.destroyFence(_immFence, nullptr); });

	// Create per-frame fences and semaphores and add to deletion queue
	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(_device.createFence(&fenceCreateInfo, nullptr, &_frames[i]._renderFence));

		vk::SemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

		VK_CHECK(_device.createSemaphore(&semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
		VK_CHECK(_device.createSemaphore(&semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));

		_mainDeletionQueue.push_function([=]() {
			_device.destroyFence(_frames[i]._renderFence, nullptr);
			_device.destroySemaphore(_frames[i]._swapchainSemaphore, nullptr);
			_device.destroySemaphore(_frames[i]._renderSemaphore, nullptr);
			});
	}

	// Initialize ready for present semaphores as per https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
	_readyForPresentSemaphores.resize(_swapchainImageCount);

	for (int i = 0; i < _swapchainImageCount; i++) {
		vk::SemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();
		VK_CHECK(_device.createSemaphore(&semaphoreCreateInfo, nullptr, &_readyForPresentSemaphores[i]));

		_mainDeletionQueue.push_function([=]() {
			_device.destroySemaphore(_readyForPresentSemaphores[i], nullptr);
			});
	}
}
//< init_sync_structures


//> cleanup
void VkSREngine::cleanup() 
{
	if (_isInitialized) {
		// Ensure that GPU has stopped all work
		_device.waitIdle();

		_mainDeletionQueue.flush();

		destroy_swapchain();

		_instance.destroySurfaceKHR(_surface);

		_allocator.destroy();
		
		_device.destroy();

		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);

		_instance.destroy();

		SDL_DestroyWindow(_window);
	}
}
//< cleanup

void VkSREngine::run() 
{
	// Nothing yet
}

