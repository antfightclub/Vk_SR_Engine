// Be careful! SDL packages its own version of stb_image... Make sure to include the right stb_image :)
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
std::optional<AllocatedImage> load_image(VkSREngine* engine, fastgltf::Asset& asset, fastgltf::Image& image) {
	AllocatedImage newImage = {};

	int width, height, nrChannels;
	std::visit(
		fastgltf::visitor{
			[](auto& arg) {},
			[&](fastgltf::sources::URI& filePath) {
			assert(filePath.fileByteOffset == 0); // Do not support byte offsets with stb_image
			assert(filePath.uri.isLocalPath());   // Only handle loading local files 
			const std::string path(filePath.uri.path().begin(), filePath.uri.path().end()); // Thanks c++
			unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
			if (data) {
				vk::Extent3D imagesize;
				imagesize.width = width;
				imagesize.height = height;
				imagesize.depth = 1;

				newImage = engine->create_image(data, imagesize, vk::Format::eR8G8B8Unorm, vk::ImageUsageFlagBits::eSampled, true); // I have not yet implemented create_image

				stbi_image_free(data);
			}
		},
		[&](fastgltf::sources::Vector& vector) {
			unsigned char* data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(vector.bytes.data()), static_cast<int>(vector.bytes.size()), &width, &height, &nrChannels, 4);
			if (data) {
				vk::Extent3D imagesize;
				imagesize.width = width;
				imagesize.height = height;
				imagesize.depth = 1;

				newImage = engine->create_image(data, imagesize, vk::Format::eR8G8B8Unorm, vk::ImageUsageFlagBits::eSampled, true);

				stbi_image_free(data);
			}
		},
		[&](fastgltf::sources::BufferView& view) {
			auto& bufferView = asset.bufferViews[view.bufferViewIndex];
			auto& buffer = asset.buffers[bufferView.bufferIndex];
			std::visit(fastgltf::visitor { // Only care about VectorWithMime here since LoadExternalBuffers has been specified meaning all buffers are already loaded into a vector
				[](auto& arg) {},
				[&](fastgltf::sources::Vector& vector) {
					unsigned char* data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(vector.bytes.data()) + bufferView.byteOffset, static_cast<int>(bufferView.byteLength), &width, &height, &nrChannels, 4);
					if (data) {
						vk::Extent3D imagesize;
						imagesize.width = width;
						imagesize.height = height;
						imagesize.depth = 1;

						newImage = engine->create_image(data, imagesize, vk::Format::eR8G8B8Unorm, vk::ImageUsageFlagBits::eSampled, true);

						stbi_image_free(data);
					}
				}
			}, buffer.data);
		},
		 
		},
		image.data);

	// If any o the attempts to load the data failed, we haven't written the image so the return handle is null
	if (newImage.image == VK_NULL_HANDLE) {
		return {};
	}
	else {
		return newImage;
	}
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
std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VkSREngine* engine, std::string_view filePath) {

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
	vk::Device dv = creator->_device;

	descriptorPool.destroy_pools(dv);
	creator->destroy_buffer(materialDataBuffer);
	
	for (auto& [k, v] : meshes) {
		creator->destroy_buffer(v->meshBuffers.indexBuffer);
		creator->destroy_buffer(v->meshBuffers.vertexBuffer);
	}

	for (auto& [k, v] : images) {
		// TODO: when adding the default error checkerboard image, remember to skip destroying it since it's a member of creator!
		fmt::println("TODO: After adding the default error checkerboard image, remember to skip destroying it since it's a member of creator!");
		creator->destroy_image(v);
	}

	for (auto& sampler : samplers) {
		dv.destroySampler(sampler, nullptr);
	}
}
//< LoadedGLTF
