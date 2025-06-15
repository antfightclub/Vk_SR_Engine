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

#include <vk_pipelines.h>

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

	init_descriptors();

	_isInitialized = true;
}

//> init_vulkan
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
//< init_vulkan

//> init_swapchain
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
//< init_swapchain

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

//> init_descriptors
void VkSREngine::init_descriptors() {
	// Create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
		{vk::DescriptorType::eStorageImage, 3},
		{vk::DescriptorType::eUniformBuffer, 3},
		{vk::DescriptorType::eCombinedImageSampler, 3},
	};

	globalDescriptorAllocator.init_pool(_device, 10, sizes);
	_mainDeletionQueue.push_function([&]() {
		_device.destroyDescriptorPool(globalDescriptorAllocator.pool, nullptr);
		});

	// Descriptor set layout for compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, vk::DescriptorType::eStorageImage);
		_drawImageDescriptorLayout = builder.build(_device, vk::ShaderStageFlagBits::eCompute);
	}

	// Descriptor set layout for gpu scene data 
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, vk::DescriptorType::eUniformBuffer);
		_gpuSceneDataDescriptorLayout = builder.build(_device, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
	}

	_mainDeletionQueue.push_function([&]() {
		_device.destroyDescriptorSetLayout(_drawImageDescriptorLayout, nullptr);
		_device.destroyDescriptorSetLayout(_gpuSceneDataDescriptorLayout, nullptr);
		});

	_drawImageDescriptors = globalDescriptorAllocator.allocate(_device, _drawImageDescriptorLayout);
	// Write the descriptor set for the draw image
	{
		DescriptorWriter writer;
		writer.write_image(0, _drawImage.imageView, VK_NULL_HANDLE, vk::ImageLayout::eGeneral, vk::DescriptorType::eStorageImage);
		writer.update_set(_device, _drawImageDescriptors);
	}

	// Create (and put into deletion queue) the per-frame descriptors
	for (int i = 0; i < FRAME_OVERLAP; i++) {
		std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
			{vk::DescriptorType::eStorageImage, 3},
			{vk::DescriptorType::eStorageBuffer, 3},
			{vk::DescriptorType::eUniformBuffer, 3},
			{vk::DescriptorType::eCombinedImageSampler, 4},
		};

		_frames[i]._frameDescriptors = DescriptorAllocatorGrowable{};
		_frames[i]._frameDescriptors.init(_device, 1000, frame_sizes);

		_mainDeletionQueue.push_function([&, i]() {
			_frames[i]._frameDescriptors.destroy_pools(_device);
			});
	}
}
//< init_descriptors

//> init_pipelines
void VkSREngine::init_pipelines() {
	init_compute_pipelines();
}

void VkSREngine::init_compute_pipelines() {
	vk::PipelineLayoutCreateInfo computeLayout = {};
	computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
	computeLayout.setLayoutCount = 1;

	vk::PushConstantRange pushConstant = {};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(ComputePushConstants);
	pushConstant.stageFlags = vk::ShaderStageFlagBits::eCompute;

	computeLayout.pPushConstantRanges = &pushConstant;
	computeLayout.pushConstantRangeCount = 1;

	VK_CHECK(_device.createPipelineLayout(&computeLayout, nullptr, &_computePipelineLayout));

	vk::ShaderModule frostyShader;
	const char* frostyPath = "../../shaders/frosty.comp.spv";
	if (!vkutil::load_shader_module(frostyPath, _device, &frostyShader)) {
		fmt::println("Error when building the shader module at path: {}", frostyPath);
	}

	vk::PipelineShaderStageCreateInfo stageInfo = {};
	stageInfo.stage = vk::ShaderStageFlagBits::eCompute;
	stageInfo.module = frostyShader;
	stageInfo.pName = "main";

	vk::ComputePipelineCreateInfo computePipelineCreateInfo = {};
	computePipelineCreateInfo.layout = _computePipelineLayout;
	computePipelineCreateInfo.stage = stageInfo;

	// Frosty
	ComputeEffect frosty;
	frosty.layout = _computePipelineLayout;
	frosty.name = "Frosty";
	frosty.data = {};

	VK_CHECK(_device.createComputePipelines(VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &frosty.pipeline));

	// Can change just the shadermodule part of computePipelineCreateInfo to create other pipelines, to reuse instead of making a new createinfo.
	// Only makes sense if all compute shaders share the same layout, though.
	_computeEffects.push_back(frosty);

	// Destroy CPU-local shader module, since it has been loaded into the GPU on createComputePipelines
	_device.destroyShaderModule(frostyShader);

	// Add pipeline to deletion queue
	_mainDeletionQueue.push_function([=]() {
		_device.destroyPipelineLayout(_computePipelineLayout, nullptr);
		// When adding new compute effects, remember to add them in opposite order for destruction to ensure valid API usage
		_device.destroyPipeline(frosty.pipeline, nullptr);
		});

}
//< init_pipelines

//> immediate_submit
void VkSREngine::immediate_submit(std::function<void(vk::CommandBuffer cmd)>&& function) {
	VK_CHECK(_device.resetFences(1, &_immFence));
	_immCommandBuffer.reset();

	vk::CommandBuffer cmd = _immCommandBuffer;
	vk::CommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	cmd.begin(cmdBeginInfo);

	function(cmd);

	cmd.end();

	vk::CommandBufferSubmitInfo cmdSubmitInfo = vkinit::command_buffer_submit_info(cmd);

	vk::SubmitInfo2 submit = vkinit::submit_info(&cmdSubmitInfo, nullptr, nullptr);

	// Submit command buffer to the queue and execute it
	// _immFence will now block until the graphic commands finish execution
	VK_CHECK(_graphicsQueue.submit2(1, &submit, _immFence));

	VK_CHECK(_device.waitForFences(1, &_immFence, true, 9999999999));
}
//< immediate_submit

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

//> draw
void VkSREngine::draw() {
	// Nothing yet
}

void VkSREngine::draw_main(vk::CommandBuffer cmd) {
	//> Compute draws
	// Get the currently chosen compute effect
	ComputeEffect& effect = _computeEffects[_currentComputeEffect];
	
	// Bind the effect's pipeline
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, effect.pipeline);

	// Bind the descriptor set containing the draw image for the compute pipeline
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _computePipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);

	// Push constants
	cmd.pushConstants(_computePipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(ComputePushConstants), &effect.data);

	// Dispatch a compute command
	cmd.dispatch(std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
	//< Compute draws
}

//< draw

void VkSREngine::run() 
{
	SDL_Event e;
	bool bQuit = false;

	// Main loop
	while (!bQuit) {

		// Handle SDL events from poll queue
		while (SDL_PollEvent(&e) != 0) {
			// Close the window when user Alt-F4's or clicks the X button 
			if (e.type == SDL_EVENT_QUIT) {
				bQuit = true;
			}

			// Handle resizing
			if (e.type == SDL_EVENT_WINDOW_RESIZED) {
				resize_requested = true;
			}

			// Stop rendering when the window is minimized
			if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
				stop_rendering = true;
			}
			if (e.type == SDL_EVENT_WINDOW_RESTORED) {
				stop_rendering = false;
			}
		}

		// Do not draw if minimized
		if (stop_rendering) {
			// Throttle speed to avoid turbo-noops
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}
		
		// Handle resizing
		if (resize_requested) {
			resize_swapchain();
		}

		// draw loop
		draw();
	}
}

