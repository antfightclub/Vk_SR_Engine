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



// Macro to check Vulkan error codes to eas use of most vulkan functions
#define VK_CHECK(x)															\
do {																		\
	VkResult err = x;														\
	if (err) {																\
		fmt::println("Detected Vulkan error: {}", string_Vkresult(err));	\
			abort();														\
	}																		\
	while (0)