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
#include "vk_mem_alloc.h"

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

	int largestWidth{ 0 };
	int largestHeight{ 0 };
	SDL_GetWindowMaximumSize(_window, &largestWidth, &largestHeight);
	_largestExtent.setHeight((uint32_t)largestHeight);
	_largestExtent.setWidth((uint32_t)largestWidth);

	init_vulkan();

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
		.require_api_version(1, 3, 0)
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	// Grab the instance and debug messenger callback
	_instance = vkb_inst.instance;
	_debug_messenger = vkb_inst.debug_messenger;

	SDL_Vulkan_CreateSurface(_window, _instance, VK_NULL_HANDLE, &_surface);

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
		.set_minimum_version(1, 4)
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
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = _chosenGPU;
	allocatorInfo.device = _device;
	allocatorInfo.instance = _instance;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaCreateAllocator(&allocatorInfo, &_allocator);
}

void VkSREngine::init_swapchain() {
	create_swapchain(_windowExtent.width, _windowExtent.height);

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
	VmaAllocationCreateInfo rimg_allocinfo = {};
	rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Allocate and create the draw image
	vmaCreateImage(_allocator, reinterpret_cast<VkImageCreateInfo*>(&rimg_info), &rimg_allocinfo, reinterpret_cast<VkImage*>(&_drawImage.image), &_drawImage.allocation, nullptr);

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
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();

	// Set _swapchainImageCount to the amount of swapchain images - used to initialize 
	// the same amount of _readyForPresentSemaphores
	VK_CHECK(vkGetSwapchainImagesKHR(_device, _swapchain, &_swapchainImageCount, nullptr));
}

void VkSREngine::destroy_swapchain() {
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);

	// Destroy swapchain resources
	for (int i = 0; i < _swapchainImageViews.size(); i++) {
		vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
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

void VkSREngine::cleanup() 
{
	if (_isInitialized) {
		// Ensure that GPU has stopped all work
		_device.waitIdle();

		_instance.destroySurfaceKHR(_surface);

		vmaDestroyAllocator(_allocator);

		_device.destroy();

		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);

		_instance.destroy();

		SDL_DestroyWindow(_window);
	}
}

void VkSREngine::run() 
{
	// Nothing yet
}

