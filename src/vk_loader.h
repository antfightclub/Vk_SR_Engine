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

//> mesh
struct Bounds {
	glm::vec3 origin;
	float sphereRadius;
	glm::vec3 extents;
};

struct GeoSurface {
	uint32_t startIndex;
	uint32_t count;
	Bounds bounds;
	std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset {
	std::string name;

	std::vector<GeoSurface> surfaces;
	GPUMeshBuffers meshBuffers;
};
//< mesh

