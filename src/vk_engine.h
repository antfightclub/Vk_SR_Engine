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

	vk::Extent2D _windowExtent{ 1920, 1080 };
	vk::Extent2D _largestExtent{ 2560, 1440 };

	struct SDL_Window* _window{ nullptr };
	static VkSREngine& Get();
	
	vk::Instance _instance;
	VkSurfaceKHR _surface;
	vk::PhysicalDevice _chosenGPU;
	vk::Device _device;
	vk::DebugUtilsMessengerEXT _debug_messenger;
	
	void init();

	void run();

	void cleanup();

private:
	void init_vulkan();
};