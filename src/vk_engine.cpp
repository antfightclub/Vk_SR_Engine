//vk_engine.cpp
#include "vk_engine.h"

// SDL3 for windowing
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vk_types.h>

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
}

void VkSREngine::cleanup() 
{
	if (_isInitialized) {
		SDL_DestroyWindow(_window);
	}
}

void VkSREngine::run() 
{
	// Nothing yet
}

