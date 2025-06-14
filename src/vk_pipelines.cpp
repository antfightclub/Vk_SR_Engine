#include <vk_pipelines.h>
#include <fstream>
#include <vk_initializers.h>
//> vkutil

bool vkutil::load_shader_module(const char* filePath, vk::Device device, vk::ShaderModule* outShaderModule) {
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		return false;
	}

	// Get file size by lookup of cursor location. Since cursor is at the end (std::ios::ate), it gives the size directly in bytes
	size_t fileSize = (size_t)file.tellg();

	// SPIR-V expects the buffer to be uint32_t so reserve a vector big enough for the entire file
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	// Put cursor at the beginning of the file
	file.seekg(0);

	// Load the entire file into the buffer
	file.read((char*)buffer.data(), fileSize);

	// Now that the file is loaded into the buffer we can close it
	file.close();

	vk::ShaderModuleCreateInfo createInfo = {};

	// CodeSize has to be in bytes so multiply the buffer size by uint32_t to get the real size of the buffer
	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	vk::ShaderModule shaderModule;
	if (device.createShaderModule(&createInfo, nullptr, &shaderModule) != vk::Result::eSuccess) {
		return false;
	}
	*outShaderModule = shaderModule;
	return true;
}

//< vkutil