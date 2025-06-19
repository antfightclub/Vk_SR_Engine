#include "stb_image.h"
#include "vk_loader.h"
#include <iostream>

#include <vk_engine.h>
#include <vk_initializers.h>
#include <vk_types.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/util.hpp>

//> global_funcs
std::optional<AllocatedImage> load_image(VulkanEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image) {
	// Nothing yet
}

vk::Filter extract_filter(fastgltf::Filter filter) {
	switch (filter) {
		// Nearest sampler
	case fastgltf::Filter::Nearest:
	case fastgltf::Filter::NearestMipMapNearest:
	case fastgltf::Filter::NearestMipMapLinear:
		return vk::Filter::eNearest;
		
		// Linear sampler (default if filter type doesn't match)
	case fastgltf::Filter::Linear:
	case fastgltf::Filter::LinearMipMapNearest:
	case fastgltf::Filter::LinearMipMapLinear:
	default:
		return vk::Filter::eLinear;
	}
}

vk::SamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter) {
	switch (filter) {
		// Nearest mipmap sampler mode
	case fastgltf::Filter::NearestMipMapNearest:
	case fastgltf::Filter::LinearMipMapNearest:
		return vk::SamplerMipmapMode::eNearest;

		// Linear mipmap sampler mode (default if mipmap filter mode doesn't match)
	case fastgltf::Filter::NearestMipMapLinear:
	case fastgltf::Filter::LinearMipMapLinear:
	default:
		return vk::SamplerMipmapMode::eLinear;
	}
} 
//< global_funcs

//> loadgltf_func
std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VulkanEngine* engine, std::string_view filePath) {

}
//< loadgltf_func

//> LoadedGLTF
void LoadedGLTF::Draw(const glm::mat4& topMatrix, DrawContext& ctx) {
	// Create renderables from the scene nodes
	for (auto& n : topNodes) {
		n->Draw(topMatrix, ctx);
	}
}

void LoadedGLTF::clearAll() {
	// Nothing yet
}
//< LoadedGLTF
