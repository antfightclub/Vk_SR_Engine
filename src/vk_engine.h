#pragma once
//vk_engine.h

#include <vk_types.h>
#include <vk_descriptors.h>
#include <vk_loader.h>
#include <camera.h>

#include "compute_structs.h"

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

struct MouseControlState{
	float mouse_saved_x;
	float mouse_saved_y;
};

// Want to implement something like this to toggle between "mouse free" and "hide mouse and lock in window for camera"
// Add ControlState as member in engine and perform event handling to toggle the state, save the mouse position for later.
// Add a keybind to toggle it. Also add a keybind to quit the application, like holding ESC for a few seconds.
/*
static SDL_Window* window;
static int saved_x, saved_y;

void setMyRelativeMouseMode(SDL_bool enable)
{
	if (enable) {
		SDL_GetMouseState(&saved_x, &saved_y);
		SDL_SetRelativeMouseMode(true);
	}
	else {
		// also try flipping the order of these calls.
		SDL_SetRelativeMouseMode(false);
		SDL_WarpMouseInWindow(window, saved_x, saved_y);
	}
}*/


struct EngineStats {
	float frametime{ 0.f };
	int triangle_count{ 0 };
	int drawcall_count{ 0 };
	float scene_update_time{ 0.f };
	float mesh_draw_time{ 0.f };
	float time_since_start{ 0.f };
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

struct GPUSceneData {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
	glm::vec4 ambientColor;
	glm::vec4 sunlightDirection; // w for sun power
	glm::vec4 sunlightColor;
};

struct GLTFMetallic_Roughness {
	MaterialPipeline opaquePipeline;
	MaterialPipeline transparentPipeline;

	vk::DescriptorSetLayout materialLayout;

	struct MaterialConstants {
		glm::vec4 colorFactors;
		glm::vec4 metal_rough_factors;
		// Paddig, need the extra space for uniform buffers
		glm::vec4 extra[14];
	};

	struct MaterialResources {
		AllocatedImage colorImage;
		vk::Sampler colorSampler;
		AllocatedImage metalRoughImage;
		vk::Sampler metalRoughSampler;
		vk::Buffer dataBuffer;
		uint32_t dataBufferOffset;
	};

	DescriptorWriter writer;

	void build_pipelines(VkSREngine* engine);
	void clear_resources(vk::Device device);

	MaterialInstance write_material(vk::Device device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);
};

struct RenderObject {
	uint32_t indexCount;
	uint32_t firstIndex;
	vk::Buffer indexBuffer;

	MaterialInstance* material;
	Bounds bounds;
	glm::mat4 transform;
	vk::DeviceAddress vertexBufferAddress;
};

struct DrawContext {
	std::vector<RenderObject> OpaqueSurfaces;
	std::vector<RenderObject> TransparentSurfaces;
}; 

struct MeshNode : public Node {
	std::shared_ptr<MeshAsset> mesh;

	virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
};

class VkSREngine {
public:
	bool _isInitialized{ false };
	int _frameNumber{ 0 };
	bool stop_rendering{ false };
	bool resize_requested{ false };

	EngineStats _stats;

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
	
	// Texture samplers
	vk::Sampler _defaultSamplerLinear;
	vk::Sampler _defaultSamplerNearest;

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
	vk::DescriptorSetLayout _gpuSceneDataDescriptorLayout;

	// Compute related
	std::vector<ComputeEffect> _computeEffects;
	int _currentComputeEffect{ 0 };
	vk::Pipeline _computePipeline;
	vk::PipelineLayout _computePipelineLayout;

	// Default images
	AllocatedImage _errorCheckerboardImage;
	AllocatedImage _blackImage;
	AllocatedImage _whiteImage;

	// Default material data
	MaterialInstance _defaultData;
	GLTFMetallic_Roughness _metalRoughMaterial;

	// Draw context 
	DrawContext _mainDrawContext;
	GPUSceneData _sceneData;

	// glTF scenes
	std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> _loadedScenes;

	// Camera
	Camera _mainCamera;

	// Controls 
	MouseControlState _mouseControlState;
	bool _is_mouse_relative{}; // bool inits to false by default

	void init();

	void run();

	void cleanup();

	void draw();
	void draw_main(vk::CommandBuffer cmd);
	void draw_geometry(vk::CommandBuffer cmd);
	void draw_imgui(vk::CommandBuffer cmd, vk::ImageView targetImageView);

	void update();
	void update_compute();
	void update_scene();
	void update_renderables();
	void update_imgui();

	void immediate_submit(std::function<void(vk::CommandBuffer cmd)>&& function);

	AllocatedBuffer create_buffer(size_t allocSize, vk::BufferUsageFlags usage, vma::MemoryUsage memoryUsage);
	AllocatedImage create_image(vk::Extent3D size, vk::Format format, vk::ImageUsageFlags usage, bool mipmapped = false);
	AllocatedImage create_image(void* data, vk::Extent3D size, vk::Format format, vk::ImageUsageFlags usage, bool mipmapped = false);
	void destroy_buffer(const AllocatedBuffer& buffer);
	void destroy_image(const AllocatedImage& img);

	GPUMeshBuffers upload_mesh(std::span<uint32_t> indices, std::span<Vertex> vertices);
	
	void handle_controls(SDL_Event& e);
	void set_relative_mouse_mode(bool enable);

private:
	void init_vulkan();
	void init_swapchain();
	void init_commands(); 
	void init_sync_structures();
	void init_descriptors();
	void init_pipelines();
	void init_compute_pipelines();
	void init_default_data();
	void init_renderables();
	void init_imgui();
	void init_controls();

	void create_swapchain(uint32_t width, uint32_t height);
	void resize_swapchain();
	void destroy_swapchain();
};