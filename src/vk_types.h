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
#include <vulkan/vk_enum_string_helper.h>

// Vulkan Memory Allocator
#include "vk_mem_alloc.h"

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


struct AllocatedImage {
	vk::Image image;
	vk::ImageView imageView;
	VmaAllocation allocation;
	vk::Extent3D imageExtent;
	vk::Format imageFormat;
};