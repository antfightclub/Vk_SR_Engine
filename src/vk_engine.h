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
	
	vk::Queue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	VmaAllocator _allocator;

	vk::SwapchainKHR _swapchain;
	vk::Format _swapchainImageFormat;

	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	vk::Extent2D _swapchainExtent;
	uint32_t _swapchainImageCount;
	std::vector<vk::Semaphore> _readyForPresentSemaphores;


	void init();

	void run();

	void cleanup();


private:
	void init_vulkan();
	void init_swapchain();

	void create_swapchain(uint32_t width, uint32_t height);
	void resize_swapchain();
	void destroy_swapchain();
};