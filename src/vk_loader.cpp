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

				newImage = engine->create_image(data, imagesize, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eSampled, true); // I have not yet implemented create_image

				stbi_image_free(data);
			}
		},
		[&](fastgltf::sources::Array& arr) {
			unsigned char* data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(arr.bytes.data()), static_cast<int>(arr.bytes.size()), &width, &height, &nrChannels, 4);
			if (data) {
				vk::Extent3D imagesize;
				imagesize.width = width;
				imagesize.height = height;
				imagesize.depth = 1;

				newImage = engine->create_image(data, imagesize, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eSampled, true);

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

						newImage = engine->create_image(data, imagesize, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eSampled, true);

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
	fmt::println("Loading GLTF: {}", filePath);

	// Prepare the LoadedGLTF structure
	std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
	scene->creator = engine;
	LoadedGLTF& file = *scene.get();

	// Initialize the fastgltf parser
	fastgltf::Parser parser{};
	constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages;

	// GltfFileStream has a buffer for storing the input filestream
	fastgltf::GltfFileStream fileStream = fastgltf::GltfFileStream(filePath);

	// Prepare a handle for later use
	fastgltf::Asset gltf;
	
	std::filesystem::path path = filePath;

	// Figure out which glTF type to load; glTF or a binary GLB
	auto type = fastgltf::determineGltfFileType(fileStream);

	if (type == fastgltf::GltfType::glTF) {
		auto load = parser.loadGltf(fileStream, path.parent_path(), gltfOptions);
		if (load) {
			gltf = std::move(load.get());
		}
		else {
			std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
			return {};
		}
	}
	else if (type == fastgltf::GltfType::GLB) {
		auto load = parser.loadGltfBinary(fileStream, path.parent_path(), gltfOptions);
		if (load) {
			gltf = std::move(load.get());
		}
		else {
			std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
			return {};
		}
	}
	else {
		std::cerr << "Failed to determine glTF container." << std::endl;
		return {};
	}

	// Time to load the gltf into the structures of LoadedGLTF.
	// Prepare descriptors
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
		{vk::DescriptorType::eCombinedImageSampler, 3},
		{vk::DescriptorType::eUniformBuffer, 3},
		{vk::DescriptorType::eStorageBuffer, 1}
	};

	file.descriptorPool.init(engine->_device, gltf.materials.size(), sizes);

	// Load samplers
	for (fastgltf::Sampler& sampler : gltf.samplers) {
		vk::SamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.maxLod = vk::LodClampNone;
		samplerCreateInfo.minLod = 0;

		samplerCreateInfo.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
		samplerCreateInfo.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		samplerCreateInfo.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		vk::Sampler newSampler;
		VK_CHECK(engine->_device.createSampler(&samplerCreateInfo, nullptr, &newSampler));

		file.samplers.push_back(newSampler);
	}

	// Temporary arrays for all the objects to use while creating the glTF data
	std::vector<std::shared_ptr<MeshAsset>> meshes;
	std::vector<std::shared_ptr<Node>> nodes;
	std::vector<AllocatedImage> images;
	std::vector<std::shared_ptr<GLTFMaterial>> materials;

	// Load all textures
	for (fastgltf::Image& image : gltf.images) {
		std::optional<AllocatedImage> img = load_image(engine, gltf, image);
		
		if (img.has_value()) {
			images.push_back(*img);
			file.images[image.name.c_str()] = *img;
		}
		else {
			// Failed to load so give assign this slot the error checkerboard texture to not completely break loading
			images.push_back(engine->_errorCheckerboardImage);
			std::cout << "glTF failed to load texture" << image.name << std::endl;
		}
	}

	// Load all materials
	// Create a buffer to hold the material data
	file.materialDataBuffer = engine->create_buffer(sizeof(GLTFMetallic_Roughness::MaterialConstants) * gltf.materials.size(), vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu);
	int data_index = 0;

	GLTFMetallic_Roughness::MaterialConstants* sceneMaterialConstants = (GLTFMetallic_Roughness::MaterialConstants*)file.materialDataBuffer.info.pMappedData;

	// Loop to load materials
	for (fastgltf::Material& mat : gltf.materials) {
		std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
		materials.push_back(newMat);
		file.materials[mat.name.c_str()] = newMat;

		GLTFMetallic_Roughness::MaterialConstants constants;
		constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
		constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
		constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
		constants.colorFactors.w = mat.pbrData.baseColorFactor[3];

		constants.metal_rough_factors.x = mat.pbrData.metallicFactor;
		constants.metal_rough_factors.y = mat.pbrData.roughnessFactor;
		
		// Write material parameters to buffer
		sceneMaterialConstants[data_index] = constants;

		MaterialPass passType = MaterialPass::MainColor;
		if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
			passType = MaterialPass::Transparent;
		}

		GLTFMetallic_Roughness::MaterialResources materialResources;
		
		// Default the material textures
		materialResources.colorImage = engine->_whiteImage;
		materialResources.colorSampler = engine->_defaultSamplerLinear;
		materialResources.metalRoughImage = engine->_whiteImage;
		materialResources.metalRoughSampler = engine->_defaultSamplerLinear;

		// Set the uniform buffer for the material data
		materialResources.dataBuffer = file.materialDataBuffer.buffer;
		materialResources.dataBufferOffset = data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);

		// Grab textures from glTF file
		if (mat.pbrData.baseColorTexture.has_value()) {
			// Holy mother of nesting
			size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
			size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

			// ...but it's neat for indexing
			materialResources.colorImage = images[img];
			materialResources.colorSampler = file.samplers[sampler];
		}

		// Build material
		
		newMat->data = engine->_metalRoughMaterial.write_material(engine->_device, passType, materialResources, file.descriptorPool);

		data_index++;	
	}

	// Loading meshes

	// Use the same vectors for all meshes so that the memory doesn't reallocate as often
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;

	// Iteratively load meshes
	for (fastgltf::Mesh& mesh : gltf.meshes) {
		std::shared_ptr<MeshAsset> newMesh = std::make_shared<MeshAsset>();
		meshes.push_back(newMesh);
		file.meshes[mesh.name.c_str()] = newMesh;
		newMesh->name = mesh.name;

		// Clear the mesh arrays to avoid crossing the beams
		indices.clear();
		vertices.clear();

		for (auto&& p : mesh.primitives) {
			GeoSurface newSurface;
			newSurface.startIndex = (uint32_t)indices.size();
			newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

			size_t initial_vtx = vertices.size();

			// Load indices
			{
				fastgltf::Accessor& indexAccessor = gltf.accessors[p.indicesAccessor.value()];
				indices.reserve(indices.size() + indexAccessor.count);

				fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAccessor,
					[&](std::uint32_t idx) {
						indices.push_back(idx + initial_vtx);
					});
			}

			// Load the vertices
			{
				fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
				vertices.resize(vertices.size() + posAccessor.count);

				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
					[&](glm::vec3 v, size_t index) {
						Vertex newVtx;
						newVtx.position = v;
						newVtx.normal = { 1,0,0 }; // Default for now, will load normals next time around if they exist
						newVtx.color = glm::vec4{ 1.f };
						newVtx.uv_x = 0;
						newVtx.uv_y = 0;
						vertices[initial_vtx + index] = newVtx;
					});
			}

			// Load vertex normals (if they exist)
			auto normals = p.findAttribute("NORMAL");
			if (normals != p.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).accessorIndex],
					[&](glm::vec3 v, size_t index) {
						vertices[initial_vtx + index].normal = v;
					});
			}

			// Load UVs (if they exist)
			auto uv = p.findAttribute("TEXCOORD_0");
			if (uv != p.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).accessorIndex],
					[&](glm::vec2 v, size_t index) {
						vertices[initial_vtx + index].uv_x = v.x;
						vertices[initial_vtx + index].uv_y = v.y;
					});
			}

			// Load vertex colors (if they exist)
			auto colors = p.findAttribute("COLOR_0");
			if (colors != p.attributes.end()) {
				fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).accessorIndex],
					[&](glm::vec4 v, size_t index) {
						vertices[initial_vtx + index].color = v;
					});
			}

			// Add material to the primitive if it exists
			if (p.materialIndex.has_value()) {
				newSurface.material = materials[p.materialIndex.value()];
			}
			else {
				newSurface.material = materials[0];
			}

			// Calculate the bounds of the mesh for culling by comparing all vertices min and max
			glm::vec3 minpos = vertices[initial_vtx].position;
			glm::vec3 maxpos = vertices[initial_vtx].position;
			for (int i = initial_vtx; i < vertices.size(); i++) {
				minpos = glm::min(minpos, vertices[i].position);
				maxpos = glm::max(maxpos, vertices[i].position);
			}
			
			// Calculate origin and extents from min/max and use the extent length for radius
			newSurface.bounds.origin = (maxpos + minpos) / 2.f;
			newSurface.bounds.extents = (maxpos - minpos) / 2.f;
			newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);

			newMesh->surfaces.push_back(newSurface);
		}

		newMesh->meshBuffers = engine->upload_mesh(indices, vertices);
	}

	// Load all nodes and their meshes
	for (fastgltf::Node& node : gltf.nodes) {
		std::shared_ptr<Node> newNode;

		// Find if the node has a mesh, and if it does hook it to the mesh pointer and allocated it with the meshnode class
		if (node.meshIndex.has_value()) {
			newNode = std::make_shared<MeshNode>();
			static_cast<MeshNode*>(newNode.get())->mesh = meshes[*node.meshIndex];
		}
		else {
			newNode = std::make_shared<Node>();
		}

		nodes.push_back(newNode);
		file.nodes[node.name.c_str()];

		

		// The transform attribute of a node is a std::variant of type either fastgltf::TRS or 
		// fastgltf::fmat4x4, and an option called "DecomposeNodeMatrices" can make node.transform
		// return a TRS. Here we handle both cases.
		std::visit(fastgltf::visitor{
			[&](fastgltf::math::fmat4x4 matrix) { 
				memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix));
			},
			[&](fastgltf::TRS transform) {
				glm::vec3 tl(transform.translation[0], transform.translation[1], transform.translation[2]);
				glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]); // w, x, y, z
				glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

				glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
				glm::mat4 rm = glm::toMat4(rot);
				glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

				newNode->localTransform = tm * rm * sm;
			}
			},
			node.transform);
	}

	// Run loop again to setup transform hierarchy
	for (int i = 0; i < gltf.nodes.size(); i++) {
		fastgltf::Node& node = gltf.nodes[i];
		std::shared_ptr<Node>& sceneNode = nodes[i];

		for (auto& c : node.children) {
			sceneNode->children.push_back(nodes[c]);
			nodes[c]->parent = sceneNode;
		}
	}

	// Find the top nodes (with no parents :c )
	for (auto& node : nodes) {
		if (node->parent.lock() == nullptr) {
			file.topNodes.push_back(node);
			node->refreshTransform(glm::mat4{ 1.f });
		}
	}
	return scene;
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
