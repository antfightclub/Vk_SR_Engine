#pragma once

#include <vk_types.h>
#include <vk_descriptors.h>
#include <unordered_map>
#include <filesystem>

// Forward declaration of the engine
class VulkanEngine;

//> material
struct GLTFMaterial {
	MaterialInstance data;
};
//< material
