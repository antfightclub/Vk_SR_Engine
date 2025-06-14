#pragma once
//vk_engine.h

#include <vk_types.h>
#include <vk_descriptors.h>

constexpr unsigned int FRAME_OVERLAP = 2;

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		// Reverse iterate the deletion queue to execute all of the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)();	 // Call functors (usually a vkdestroysomething)
		}

		deletors.clear();
	}
};

struct FrameData 
{
	vk::Semaphore _swapchainSemaphore, _renderSemaphore;
	vk::Fence _renderFence;

	vk::CommandPool _commandPool;
	vk::CommandBuffer _mainCommandBuffer;

	DeletionQueue _deletionQueue;
	DescriptorAllocatorGrowable _frameDescriptors;
};

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
	
	// Vulkan instance and device related
	vk::Instance _instance;
	vk::SurfaceKHR _surface;
	vk::PhysicalDevice _chosenGPU;
	vk::Device _device;
	vk::DebugUtilsMessengerEXT _debug_messenger;
	vk::Queue _graphicsQueue;
	uint32_t _graphicsQueueFamily;
	
	// Allocation and deletion
	DeletionQueue _mainDeletionQueue;
	vma::Allocator _allocator;

	// Swapchain
	vk::SwapchainKHR _swapchain;
	vk::Format _swapchainImageFormat;

	std::vector<vk::Image> _swapchainImages;
	std::vector<vk::ImageView> _swapchainImageViews;
	vk::Extent2D _swapchainExtent;
	uint32_t _swapchainImageCount;
	std::vector<vk::Semaphore> _readyForPresentSemaphores;

	// Draw and depth images
	AllocatedImage _drawImage;
	AllocatedImage _depthImage;
	vk::Extent2D _drawExtent;
	float renderScale = 1.f;


	// Per frame structures
	FrameData _frames[FRAME_OVERLAP];
	FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; };

	// Immediate submit structures
	vk::Fence _immFence;
	vk::CommandBuffer _immCommandBuffer;
	vk::CommandPool _immCommandPool;

	// Descriptors
	DescriptorAllocator globalDescriptorAllocator;
	vk::DescriptorSet _drawImageDescriptors;
	vk::DescriptorSetLayout _drawImageDescriptorLayout;

	void init();

	void run();

	void cleanup();

	void immediate_submit(std::function<void(vk::CommandBuffer cmd)>&& function);

private:
	void init_vulkan();
	void init_swapchain();
	void init_commands(); 
	void init_sync_structures();
	void init_descriptors();

	void create_swapchain(uint32_t width, uint32_t height);
	void resize_swapchain();
	void destroy_swapchain();
};