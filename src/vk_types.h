#pragma once
//vk_types.h

// Standard library includes for various functionality across the engine (bite me)
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

// Vulkan includes
#include <vulkan/vulkan.hpp> // using the HPP bindings
#include <vulkan/vulkan_to_string.hpp>

// Vulkan Memory Allocator
#include "vk_mem_alloc.hpp"

// fmt library for better console printouts
#include <fmt/core.h>

// GL Math likely to be used across the engine
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

// Macro to check Vulkan error codes to eas use of most vulkan functions
#define VK_CHECK(x)															\
do {																		\
	vk::Result err = x;														\
	if (err != vk::Result::eSuccess) {																\
		fmt::println("Detected Vulkan error: {}", to_string(err));	\
			abort();														\
		}																	\
} while (0)

//> allocated
struct AllocatedImage {
	vk::Image image;
	vk::ImageView imageView;
	vma::Allocation allocation;
	vk::Extent3D imageExtent;
	vk::Format imageFormat;
};

struct AllocatedBuffer {
	vk::Buffer buffer;
	vma::Allocation allocation;
	vma::AllocationInfo info;
};
//< allocated

//> mesh
struct Vertex {
	glm::vec3 postion;
	float uv_x;
	glm::vec3 normal;
	float uv_y;
	glm::vec4 color;
};

struct GPUMeshBuffers {
	AllocatedBuffer indexBuffer;
	AllocatedBuffer vertexBuffer;
	vk::DeviceAddress vertexBufferAddress;
};

struct GPUDrawPushConstants {
	glm::mat4 worldMatrix;
	vk::DeviceAddress vertexBuffer;
};
//< mesh

//> material
enum class MaterialPass : uint8_t {
	MainColor,
	Transparent,
	Other
};

struct MaterialPipeline {
	vk::Pipeline pipeline;
	vk::Pipeline layout;
};

struct MaterialInstance {
	MaterialPipeline* pipeline;
	vk::DescriptorSet materialSet;
	MaterialPass passType;
};
//< material

// Forward declaration for DrawContext 
struct DrawContext;

//> renderables
// Base class for a renderable dynamic object
class IRenderable {
	virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
};

// Implementation of a drawable scene node
// The scene node can hold children and will also keep a transform to propagate to them
struct Node : public IRenderable {

	// Parent pointer must be a weak pointer to avoid circular dependencies
	std::weak_ptr<Node> parent;
	std::vector<std::shared_ptr<Node>> children;

	glm::mat4 localTransform;
	glm::mat4 worldTransform;

	void refreshTransform(const glm::mat4& parentMatrix) {
		worldTransform = parentMatrix * localTransform;
		for (auto c : children) {
			c->refreshTransform(worldTransform);
		}
	}

	virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) {
		// Draw children
		for (auto& c : children) {
			c->Draw(topMatrix, ctx);
		}
	}
};

//< renderables