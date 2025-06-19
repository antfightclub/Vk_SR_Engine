#pragma once

#include <vk_types.h>
#include <vk_descriptors.h>
#include <unordered_map>
#include <filesystem>

// Forward declaration of the engine
class VkSREngine;

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

//> gltf
struct LoadedGLTF : public IRenderable {
	// Storage for all the data on a given gLTF file
	std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
	std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
	std::unordered_map<std::string, AllocatedImage> images;
	std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

	// Nodes that do not have a parent, for iterating the file tree in order
	std::vector<std::shared_ptr<Node>> topNodes;

	std::vector<vk::Sampler> samplers;

	DescriptorAllocatorGrowable descriptorPool;

	AllocatedBuffer materialDataBuffer;

	VkSREngine* creator;

	~LoadedGLTF() { clearAll(); };

	virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx);

private:
	void clearAll();
};

std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VkSREngine* engine, std::string_view filePath);
//< gltf

