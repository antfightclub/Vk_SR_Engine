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

#include <vk_images.h>
#include <vk_pipelines.h>

#include <chrono>
#include <thread>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

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
	
	init_pipelines();
	
	init_default_data();
	
	init_renderables();

	_mainCamera.velocity = glm::vec3{ 0.f };
	_mainCamera.position = glm::vec3{ 0.f, 0.f, 0.f };
	_mainCamera.pitch = 0;
	_mainCamera.yaw = 0;

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

	_metalRoughMaterial.build_pipelines(this);
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

//> init_default_data
void VkSREngine::init_default_data() {
	// Create a one-pixel texture which is black
	uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
	_blackImage = create_image((void*)&black, vk::Extent3D{ 1, 1, 1 }, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eSampled);

	uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
	_whiteImage = create_image((void*)&white, vk::Extent3D{ 1,1,1 }, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eSampled);

	// Create a magenta and black checkerboard image to put in place of any failed texture loads
	uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	std::array<uint32_t, 16 * 16> pixels; // 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}

	_errorCheckerboardImage = create_image(pixels.data(), vk::Extent3D{ 16, 16, 1 }, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eSampled);

	// Initialize the two default samplers
	vk::SamplerCreateInfo sampl = {};
	sampl.magFilter = vk::Filter::eNearest;
	sampl.minFilter = vk::Filter::eNearest;

	VK_CHECK(_device.createSampler(&sampl, nullptr, &_defaultSamplerNearest));

	sampl.magFilter = vk::Filter::eLinear;
	sampl.minFilter = vk::Filter::eLinear;

	VK_CHECK(_device.createSampler(&sampl, nullptr, &_defaultSamplerLinear));
	
	// Cleanup
	_mainDeletionQueue.push_function([&]() {
		_device.destroySampler(_defaultSamplerLinear);
		_device.destroySampler(_defaultSamplerNearest);

		destroy_image(_blackImage);
		destroy_image(_whiteImage);
		destroy_image(_errorCheckerboardImage);
		});
}

void VkSREngine::init_renderables() {
}
//< init_default_data

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

		_loadedScenes.clear();

		for (auto& frame : _frames) {
			frame._deletionQueue.flush();
		}

		_metalRoughMaterial.clear_resources(_device);

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
bool is_visible(const RenderObject& obj, const glm::mat4& viewProj) {
	// Simple culling function 
	// Define the 8 corners of the normalized device coordinates (NDC) or clip space, the space objects are in after view and projection
	std::array<glm::vec3, 8> corners{
		glm::vec3 { 1, 1, 1 },
		glm::vec3 { 1, 1, -1 },
		glm::vec3 { 1, -1, 1 },
		glm::vec3 { 1, -1, -1 },
		glm::vec3 { -1, 1, 1 },
		glm::vec3 { -1, 1, -1 },
		glm::vec3 { -1, -1, 1 },
		glm::vec3 { -1, -1, -1 },
	};

	// Get object's position in NDC
	glm::mat4 matrix = viewProj * obj.transform;

	// Min and max bounds as the two corners of a box
	glm::vec3 min = { 1.5, 1.5, 1.5 };
	glm::vec3 max = { -1.5, -1.5, -1.5 };

	for (int c = 0; c < 8; c++) {
		// Project each corner into clip space
		glm::vec4 v = matrix * glm::vec4(obj.bounds.origin + (corners[c] * obj.bounds.extents), 1.f);

		// Perspective correction
		v.x = v.x / v.w;
		v.y = v.y / v.w;
		v.z = v.z / v.w;

		min = glm::min(glm::vec3{ v.x, v.y, v.z }, min);
		max = glm::max(glm::vec3{ v.x, v.y, v.z }, max);
	}

	// Check whether the clip space box is within view
	if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f) {
		return false;
	}
	else {
		return true;
	}
}


void VkSREngine::draw() {
	// Wait until the GPU has finished rendering the last frame. Timout of 1 second
	VK_CHECK(_device.waitForFences(1, &get_current_frame()._renderFence, true, 1000000000));

	// Flush per frame data
	get_current_frame()._deletionQueue.flush();
	get_current_frame()._frameDescriptors.clear_pools(_device);

	// Request an image from the swapchain
	uint32_t swapchainImageIndex;

	// Handle window resizing
	vk::Result e = _device.acquireNextImageKHR(_swapchain, 1000000000, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex);
	if (e == vk::Result::eErrorOutOfDateKHR) {
		resize_requested = true;
		return;
	}

	// Update draw image extent
	_drawExtent.height = std::min(_swapchainExtent.height, _drawImage.imageExtent.height) * renderScale;
	_drawExtent.width = std::min(_swapchainExtent.width, _drawImage.imageExtent.width) * renderScale;
	
	// Reset frame renderfence
	VK_CHECK(_device.resetFences(1, &get_current_frame()._renderFence));

	// Reset frame command buffer
	get_current_frame()._mainCommandBuffer.reset();

	// Get the cmd handle for ease of use
	vk::CommandBuffer cmd = get_current_frame()._mainCommandBuffer;

	// Begin command buffer
	vk::CommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	VK_CHECK(cmd.begin(&cmdBeginInfo));


	// Transition draw image and depth image into general layout so that we can write into it.
	// It will all be overwritten so don't care about the older layout.
	vkutil::transition_image(cmd, _drawImage.image, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
	vkutil::transition_image(cmd, _depthImage.image, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal);

	draw_main(cmd);

	// Transition the draw image and swapchain image into their correct transfer layouts
	vkutil::transition_image(cmd, _drawImage.image, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal);
	vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

	VkExtent2D extent;
	extent.height = _windowExtent.height;
	extent.width = _windowExtent.width;

	// Execute a copy from the draw image into the swapchain
	vkutil::copy_image_to_image(cmd, _drawImage.image, _swapchainImages[swapchainImageIndex], _drawExtent, _swapchainExtent);

	// Transition from transferdst to color attachment for future compatibility when using immediate submits with imgui
	vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal);

	// Set swapchain image layout to present so we can show it to the window
	vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);

	// Finalize the command buffer (can no longer add commands, but it can now be executed)
	cmd.end();


	// Prepare the submission to the queue.
	// We want to wait on the _presentSemaphore as it is signaled when the swapchain is ready.
	// We will signal the _readyForPresentSemaphores at the current swapchain index to signal that rendering has finished
	vk::CommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);

	vk::SemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(vk::PipelineStageFlagBits2::eColorAttachmentOutput, get_current_frame()._swapchainSemaphore);
	vk::SemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(vk::PipelineStageFlagBits2::eAllGraphics, _readyForPresentSemaphores[swapchainImageIndex]);
	
	vk::SubmitInfo2 submit = vkinit::submit_info(&cmdInfo, &signalInfo, &waitInfo);

	// Submit command buffer to the queue and execute it
	// _renderFence will now block until the graphics commands finish execution
	VK_CHECK(_graphicsQueue.submit2(1, &submit, get_current_frame()._renderFence));

	// Prepare for presentation
	// This will put the image we just rendered into the visible window
	// we want to wait on the _readyForPresent semaphore for that, as it is
	// necessary that drawing commands have finished before the image is displayed
	vk::PresentInfoKHR presentInfo = vkinit::present_info();

	presentInfo.pSwapchains = &_swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &_readyForPresentSemaphores[swapchainImageIndex];
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	vk::Result presentResult = _graphicsQueue.presentKHR(&presentInfo);
	if (presentResult == vk::Result::eErrorOutOfDateKHR) {
		resize_requested = true;
		return;
	}

	// Increase number of frames drawn
	_frameNumber++;
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

	//> geometry draws
	vk::RenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_drawImage.imageView, nullptr, vk::ImageLayout::eGeneral);
	vk::RenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_depthImage.imageView, vk::ImageLayout::eDepthAttachmentOptimal);

	vk::RenderingInfo renderInfo = vkinit::rendering_info(_windowExtent, &colorAttachment, &depthAttachment);

	cmd.beginRendering(&renderInfo);

	auto start = std::chrono::system_clock::now();
	
	draw_geometry(cmd);

	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	_stats.mesh_draw_time = elapsed.count() / 1000.f;

	cmd.endRendering();
	//< geometry draws
}

void VkSREngine::draw_geometry(vk::CommandBuffer cmd) {
	std::vector<uint32_t> opaque_draws;

	opaque_draws.reserve(_mainDrawContext.OpaqueSurfaces.size());

	// Perform culling, that is decide which surfaces should be drawn depending on if they are in view
	for (uint32_t i = 0; i < _mainDrawContext.OpaqueSurfaces.size(); i++) {
		//if (is_visible(_mainDrawContext.OpaqueSurfaces[i], _sceneData.viewproj)) {
		//	opaque_draws.push_back(i);
		//}
		opaque_draws.push_back(i);
	}

	// Sort the opaque surfaces by material and mesh
	std::sort(opaque_draws.begin(), opaque_draws.end(), [&](const auto& iA, const auto& iB) {
		const RenderObject& A = _mainDrawContext.OpaqueSurfaces[iA];
		const RenderObject& B = _mainDrawContext.OpaqueSurfaces[iB];
		if (A.material == B.material) {
			return A.indexBuffer < B.indexBuffer;
		}
		else {
			return A.material < B.material;
		}
		});

	// Allocate a new uniform buffer for the scene data
	AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData), vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu);

	// Add it to the deletion queue of this frame so it gets deleted once it's been used
	get_current_frame()._deletionQueue.push_function([=, this]() {
		destroy_buffer(gpuSceneDataBuffer);
		});

	// Write the buffer
	GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.info.pMappedData; // would this work? No idea! It should have been from the "allocation" instead of the "allocationinfo" when not the hpp headers.
	*sceneUniformData = _sceneData;

	// Create a descriptor set which binds the buffer and updates it
	vk::DescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout);

	DescriptorWriter writer;
	writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, vk::DescriptorType::eUniformBuffer);
	writer.update_set(_device, globalDescriptor);

	MaterialPipeline* lastPipeline = nullptr;
	MaterialInstance* lastMaterial = nullptr;
	vk::Buffer lastIndexBuffer = VK_NULL_HANDLE;

	//> Draw lambda
	auto draw = [&](const RenderObject& r) {
		if (r.material != lastMaterial) {
			lastMaterial = r.material;
			// Rebind pipeline and descriptors if the material changed
			if (r.material->pipeline != lastPipeline) {
				lastPipeline = r.material->pipeline;

				cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, r.material->pipeline->pipeline);
				cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, r.material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);

				// Setup viewport
				vk::Viewport viewport = {};
				viewport.x = 0;
				viewport.y = 0;
				viewport.width = (float)_windowExtent.width;
				viewport.height = (float)_windowExtent.height;
				viewport.minDepth = 0.f;
				viewport.maxDepth = 1.f;

				cmd.setViewport(0, 1, &viewport);

				// Setup scissor
				vk::Rect2D scissor = {};
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				scissor.extent.width = _windowExtent.width;
				scissor.extent.height = _windowExtent.height;

				cmd.setScissor(0, 1, &scissor);
			}
			// Bind material descriptor sets
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, r.material->pipeline->layout, 1, 1, &r.material->materialSet, 0, nullptr);
		}
		// Rebind index buffer if needed

		if (r.indexBuffer != lastIndexBuffer) {
			lastIndexBuffer = r.indexBuffer;
			cmd.bindIndexBuffer(r.indexBuffer, 0, vk::IndexType::eUint32);
		}

		// Calculate final mesh matrix
		GPUDrawPushConstants push_constants;
		push_constants.worldMatrix = r.transform;
		push_constants.vertexBuffer = r.vertexBufferAddress;
		cmd.pushConstants(r.material->pipeline->layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUDrawPushConstants), &push_constants);
	
		// Perform the actual draw call
		cmd.drawIndexed(r.indexCount, 1, r.firstIndex, 0, 0);

		// Update stats counters
		_stats.drawcall_count++;
		_stats.triangle_count += r.indexCount / 3;	
		};
	//< draw_lambda

	// Reset stats counters
	_stats.drawcall_count = 0;
	_stats.triangle_count = 0;

	// Use the draw lambda to first draw opaque surfaces then transparent surfaces
	for (auto& r : opaque_draws) {
		draw(_mainDrawContext.OpaqueSurfaces[r]);
	}
	
	for (auto& r : _mainDrawContext.TransparentSurfaces) {
		draw(r);
	}
	
	// Delete draw commands now that we processed them
	_mainDrawContext.OpaqueSurfaces.clear();
	_mainDrawContext.TransparentSurfaces.clear();
}
//< draw

//> buffer/image/mesh allocation
AllocatedBuffer VkSREngine::create_buffer(size_t allocSize, vk::BufferUsageFlags usage, vma::MemoryUsage memoryUsage) {
	// Vulkan create info
	vk::BufferCreateInfo bufferInfo = {};
	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;

	// vma create info
	vma::AllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;
	vmaallocInfo.flags = vma::AllocationCreateFlagBits::eMapped;
	
	AllocatedBuffer newBuffer;

	// Allocate the buffer
	VK_CHECK(_allocator.createBuffer(&bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info));

	return newBuffer;
}

AllocatedImage VkSREngine::create_image(vk::Extent3D size, vk::Format format, vk::ImageUsageFlags usage, bool mipmapped) {
	AllocatedImage newImage;
	newImage.imageFormat = format;
	newImage.imageExtent = size;

	vk::ImageCreateInfo img_info = vkinit::image_create_info(format, usage, size);
	if (mipmapped) {
		img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// Always allocate images on dedicated GPU memory
	vma::AllocationCreateInfo allocInfo = {};
	allocInfo.usage = vma::MemoryUsage::eGpuOnly;
	allocInfo.requiredFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

	// Allocate and create the image
	VK_CHECK(_allocator.createImage(&img_info, &allocInfo, &newImage.image, &newImage.allocation, nullptr));

	// If the format is a depth format, use the correct aspect flag
	vk::ImageAspectFlags aspectFlag = vk::ImageAspectFlagBits::eColor;
	if (format == vk::Format::eD32Sfloat) {
		aspectFlag = vk::ImageAspectFlagBits::eDepth;
	}

	// Build an image view for the image
	vk::ImageViewCreateInfo view_info = vkinit::imageview_create_info(format, newImage.image, aspectFlag);
	view_info.subresourceRange.levelCount = img_info.mipLevels;

	// Create the image view
	VK_CHECK(_device.createImageView(&view_info, nullptr, &newImage.imageView));

	return newImage;
}

AllocatedImage VkSREngine::create_image(void* data, vk::Extent3D size, vk::Format format, vk::ImageUsageFlags usage, bool mipmapped) {
	// As we're using a void pointer, calculate the size of the data from the vk::Extent3D and the number of channels (4, rgba) 
	size_t data_size = size.depth * size.width * size.height * 4;
	
	// Use a staging buffer for copying the image into gpu memory using immediate submit
	AllocatedBuffer uploadBuffer = create_buffer(data_size, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuToGpu);

	memcpy(uploadBuffer.info.pMappedData, data, data_size);

	// Use the other overload of create_image
	AllocatedImage new_image = create_image(size, format, usage | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, mipmapped);
	
	// Use immediate_submit to submit the image to GPU memory
	immediate_submit([&](vk::CommandBuffer cmd) {
		vkutil::transition_image(cmd, new_image.image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

		vk::BufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = size;

		// Copy buffer into the image
		cmd.copyBufferToImage(uploadBuffer.buffer, new_image.image, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);

		if (mipmapped) {
			// generate_mipmaps transitions the from eTransfferDstOptimal to eShaderReadOnlyOptimal
			vkutil::generate_mipmaps(cmd, new_image.image, vk::Extent2D{ new_image.imageExtent.width, new_image.imageExtent.height });
		}
		else {
			vkutil::transition_image(cmd, new_image.image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
		}
		});
	
	// Free the staging buffer
	destroy_buffer(uploadBuffer);
	return new_image;
}

void VkSREngine::destroy_buffer(const AllocatedBuffer& buffer) {
	_allocator.destroyBuffer(buffer.buffer, buffer.allocation);
}

void VkSREngine::destroy_image(const AllocatedImage& img) {
	_device.destroyImageView(img.imageView, nullptr);
	_allocator.destroyImage(img.image, img.allocation);
}

GPUMeshBuffers VkSREngine::upload_mesh(std::span<uint32_t> indices, std::span<Vertex> vertices) {
	const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

	GPUMeshBuffers newSurface;

	// Create vertex buffer
	newSurface.vertexBuffer = create_buffer(vertexBufferSize, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress, vma::MemoryUsage::eGpuOnly);

	// Find the adress of the vertex buffer
	vk::BufferDeviceAddressInfo deviceAddressInfo = {};
	deviceAddressInfo.buffer = newSurface.vertexBuffer.buffer;
	
	newSurface.vertexBufferAddress = _device.getBufferAddress(&deviceAddressInfo);

	// Create index buffer
	newSurface.indexBuffer = create_buffer(indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vma::MemoryUsage::eGpuOnly);

	// Use a CPU-local staging buffer which will get copied into GPU memory
	AllocatedBuffer staging = create_buffer(vertexBufferSize + indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuOnly);

	void* data = staging.info.pMappedData; // In VMA, the VmaAllocation would have a pointer ->GetMappedData(), but in VMA-HPP, that is not present. vma::AllocationInfo.pMappedData has the same effect in VMA-HPP it seems

	// Copy vertex buffer
	memcpy(data, vertices.data(), vertexBufferSize);
	// Copy index buffer
	memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

	// Use immediatesubmit to copy buffers to GPU
	immediate_submit([&](vk::CommandBuffer cmd) {
		vk::BufferCopy vertexCopy{ 0 };
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = vertexBufferSize;

		cmd.copyBuffer(staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

		vk::BufferCopy indexCopy{ 0 };
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertexBufferSize;
		indexCopy.size = indexBufferSize;

		cmd.copyBuffer(staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);

		});

	destroy_buffer(staging);

	return newSurface;
}
//< buffer/image/mesh allocation


//> update
void VkSREngine::update() {
	update_compute();

	update_scene();
}

void VkSREngine::update_compute() {
	// Advance time in frosty shader
	_computeEffects[0].data.data1.x = _stats.time_since_start;
}

void VkSREngine::update_scene() {
	// Begin clock
	auto start = std::chrono::system_clock::now();

	_mainCamera.update();

	glm::mat4 view = _mainCamera.getViewMatrix();

	// Camera projection 
	// The "near" and "far" parameters are flipped to get a reversed depth range, see https://developer.nvidia.com/blog/visualizing-depth-precision/
	glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)_windowExtent.width / (float)_windowExtent.height, 10000.f, 0.1f); 

	// Invert the Y direction on the projection matrix to conform to OpenGL and glTF axis conventions
	projection[1][1] *= -1;

	_mainDrawContext.OpaqueSurfaces.clear();

	update_renderables();

	_sceneData.view = view;
	_sceneData.proj = projection;
	_sceneData.viewproj = projection * view; // the GLM order of operations is "backwards" compared to GLSL due to conventions

	// Some default lighting parameters
	_sceneData.ambientColor = glm::vec4{ 1.f };
	_sceneData.sunlightColor = glm::vec4{ 1.f };
	_sceneData.sunlightDirection = glm::vec4{ 0, 1, 0.5, 1.f }; // w is intensity

	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	_stats.scene_update_time = elapsed.count() / 1000.f;
}

void VkSREngine::update_renderables() {
}
//< update

void VkSREngine::run() 
{
	SDL_Event e;
	bool bQuit = false;

	// Main loop
	while (!bQuit) {

		// Begin clock
		auto start = std::chrono::system_clock::now();

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

			// Camera movement
			_mainCamera.processSDLEvent(e);
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

		// Update
		update();

		// draw loop
		draw();
		
		// Get end time
		auto end = std::chrono::system_clock::now();

		// Convert to microseconds (int) and then back to milliseconds
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		_stats.frametime = elapsed.count() / 1000.f;
		// Then back to seconds
		_stats.time_since_start += _stats.frametime / 1000.f;
	}
}

// ############## GLTF materials ###############
void GLTFMetallic_Roughness::build_pipelines(VkSREngine* engine) {
	// Fixed function pipeline vertex shader and fragment shader
	vk::ShaderModule meshVertexShader;
	if (!vkutil::load_shader_module("../../shaders/mesh.vert.spv", engine->_device, &meshVertexShader)) {
		fmt::println("Error when building the vertex shader module!");
	}

	vk::ShaderModule meshFragmentShader;
	if (!vkutil::load_shader_module("../../shaders/mesh.frag.spv", engine->_device, &meshFragmentShader)) {
		fmt::println("Error when building the fragment shader module!");
	}

	vk::PushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = vk::ShaderStageFlagBits::eVertex;

	DescriptorLayoutBuilder layoutBuilder;
	layoutBuilder.add_binding(0, vk::DescriptorType::eUniformBuffer);
	layoutBuilder.add_binding(1, vk::DescriptorType::eCombinedImageSampler);
	layoutBuilder.add_binding(2, vk::DescriptorType::eCombinedImageSampler);

	materialLayout = layoutBuilder.build(engine->_device, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorSetLayout layouts[] = { engine->_gpuSceneDataDescriptorLayout, materialLayout };
	
	vk::PipelineLayoutCreateInfo meshLayoutInfo = vkinit::pipeline_layout_create_info();
	meshLayoutInfo.setLayoutCount = 2;
	meshLayoutInfo.pSetLayouts = layouts;
	meshLayoutInfo.pPushConstantRanges = &matrixRange;
	meshLayoutInfo.pushConstantRangeCount = 1;

	vk::PipelineLayout newLayout;
	VK_CHECK(engine->_device.createPipelineLayout(&meshLayoutInfo, nullptr, &newLayout));

	opaquePipeline.layout = newLayout;
	transparentPipeline.layout = newLayout;

	// Build the stage create info for both vertex and fragment stages, which tells the pipeline which shader modules to uge per stage
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.set_shaders(meshVertexShader, meshFragmentShader);
	pipelineBuilder.set_input_topology(vk::PrimitiveTopology::eTriangleList);
	pipelineBuilder.set_polygon_mode(vk::PolygonMode::eFill);
	pipelineBuilder.set_cull_mode(vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise);
	pipelineBuilder.set_multisampling_none();
	pipelineBuilder.disable_blending();
	pipelineBuilder.enable_depthtest(true, vk::CompareOp::eGreaterOrEqual);

	// Render format
	pipelineBuilder.set_color_attachment_format(engine->_drawImage.imageFormat);
	pipelineBuilder.set_depth_format(engine->_depthImage.imageFormat);

	// Use the triangle layout
	pipelineBuilder._pipelineLayout = newLayout;

	// Build the pipeline
	opaquePipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);

	// Create the transparent variant for transparent surfaces
	pipelineBuilder.enable_blending_additive();
	
	pipelineBuilder.enable_depthtest(false, vk::CompareOp::eGreaterOrEqual);

	transparentPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device);

	engine->_device.destroyShaderModule(meshFragmentShader, nullptr);
	engine->_device.destroyShaderModule(meshVertexShader, nullptr);

}

void GLTFMetallic_Roughness::clear_resources(vk::Device device) {
	device.destroyDescriptorSetLayout(materialLayout, nullptr);
	device.destroyPipelineLayout(transparentPipeline.layout, nullptr);
	
	device.destroyPipeline(transparentPipeline.pipeline, nullptr);
	device.destroyPipeline(opaquePipeline.pipeline, nullptr);
}

MaterialInstance GLTFMetallic_Roughness::write_material(vk::Device device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator) {
	MaterialInstance matData;
	matData.passType = pass;
	if (pass == MaterialPass::Transparent) {
		matData.pipeline = &transparentPipeline;
	}
	else {
		matData.pipeline = &opaquePipeline;
	}

	matData.materialSet = descriptorAllocator.allocate(device, materialLayout);

	writer.clear();
	writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, vk::DescriptorType::eUniformBuffer);
	writer.write_image(1, resources.colorImage.imageView, resources.colorSampler, vk::ImageLayout::eShaderReadOnlyOptimal, vk::DescriptorType::eCombinedImageSampler);
	writer.write_image(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, vk::ImageLayout::eShaderReadOnlyOptimal, vk::DescriptorType::eCombinedImageSampler);

	writer.update_set(device, matData.materialSet);

	return matData;
}


// ############## MeshNode ###############
void MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx) {
	glm::mat4 nodeMatrix = topMatrix * worldTransform;

	for (auto& s : mesh->surfaces) {
		RenderObject def;
		def.indexCount = s.count;
		def.firstIndex = s.startIndex;
		def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
		def.material = &s.material->data;
		def.bounds = s.bounds;
		def.transform = nodeMatrix;
		def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;

		if (s.material->data.passType == MaterialPass::Transparent) {
			ctx.TransparentSurfaces.push_back(def);
		}
		else {
			ctx.OpaqueSurfaces.push_back(def);
		}
	}

	// Recurse down
	Node::Draw(topMatrix, ctx);
}