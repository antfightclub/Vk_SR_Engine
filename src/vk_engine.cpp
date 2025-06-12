//vk_engine.cpp
#include "vk_engine.h"

#include <chrono>
#include <thread>

#include <iostream>

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

	std::cout << "Hello, VkSREngine!" << std::endl;
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

