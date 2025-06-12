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

	struct SDL_Window* _window{ nullptr };
	static VkSREngine& Get();
	
	void init();

	void run();

	void cleanup();

private:
	void init_vulkan();
};