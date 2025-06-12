//vk_engine.cpp
#include "vk_engine.h"

#include <chrono>
#include <thread>

#include <iostream>

// SDL3 for windowing
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

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

	SDL_Log("%s", "Hello, VkSREngine with SDL3!");
}

void VkSREngine::init_vulkan() 
{
	// Nothing yet
}

void VkSREngine::cleanup() 
{
	// Nothing yet
}

void VkSREngine::run() 
{
	// Nothing yet
}

