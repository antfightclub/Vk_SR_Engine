#pragma once
//vk_engine.h

#include <vk_types.h>

constexpr unsigned int FRAME_OVERLAP = 2;

class VkSREngine {
public:
	bool _isInitialized{ false };
	int _frameNumber{ 0 };
	bool stop_rendering{ false };
	bool resize_requested{ false };

	VkExtent2D _windowExtent{ 1920, 1080 };
	VkExtent2D _largestExtent{ 2560, 1440 };

	struct SDL_Window* _window{ nullptr };
	static VkSREngine& Get();

	VkInstance _instance;
	VkSurfaceKHR _surface;
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;
	VkDebugUtilsMessengerEXT _debug_messenger;
	
	void init();

	void run();

	void cleanup();

private:
	void init_vulkan();
};