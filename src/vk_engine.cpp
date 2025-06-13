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
	// Nothing yet
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

