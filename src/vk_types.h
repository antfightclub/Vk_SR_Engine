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
	VkResult err = x;														\
	if (err) {																\
		fmt::println("Detected Vulkan error: {}", string_VkResult(err));	\
			abort();														\
		}																	\
} while (0)