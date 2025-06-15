#include <vk_pipelines.h>
#include <fstream>
#include <vk_initializers.h>

//> pipelinebuilder
void PipelineBuilder::clear() {
	// Clear all of the structs back to zero-initialized
	_inputAssembly = vk::PipelineInputAssemblyStateCreateInfo{};

	_rasterizer = vk::PipelineRasterizationStateCreateInfo{};

	_colorBlendAttachment = vk::PipelineColorBlendAttachmentState{};

	_multisampling = vk::PipelineMultisampleStateCreateInfo{};
	
	_depthStencil = vk::PipelineDepthStencilStateCreateInfo{};

	_renderInfo = vk::PipelineRenderingCreateInfo{};

	_shaderStages.clear();
}

vk::Pipeline PipelineBuilder::build_pipeline(vk::Device device) {
	// Make viewport state from the stored viewport and scissor
	// Does not support multiple viewports and scissors

	vk::PipelineViewportStateCreateInfo viewportState = {};

	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// Setup color blending (dummy)
	vk::PipelineColorBlendStateCreateInfo colorBlending = {};

	colorBlending.logicOpEnable = vk::False;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &_colorBlendAttachment;

	// Completely clear vertex input state create info, there is no need for it
	vk::PipelineVertexInputStateCreateInfo _vertexInputInfo = {};

	// Build the actual pipeline
	vk::GraphicsPipelineCreateInfo pipelineInfo = {};

	pipelineInfo.pNext = &_renderInfo;

	pipelineInfo.stageCount = (uint32_t)_shaderStages.size();
	pipelineInfo.pStages = _shaderStages.data();
	
	pipelineInfo.pVertexInputState = &_vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &_inputAssembly;
	pipelineInfo.pViewportState = &viewportState;

	pipelineInfo.pRasterizationState = &_rasterizer;
	pipelineInfo.pMultisampleState = &_multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;

	pipelineInfo.pDepthStencilState = &_depthStencil;

	pipelineInfo.layout = _pipelineLayout;

	vk::DynamicState state[] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

	vk::PipelineDynamicStateCreateInfo dynamicInfo = {};
	dynamicInfo.pDynamicStates = &state[0];
	dynamicInfo.dynamicStateCount = 2;

	pipelineInfo.pDynamicState = &dynamicInfo;

	// It's easy to error out on creating graphics pipeline, so handle it a bit better than the common VK_CHECK case
	vk::Pipeline newPipeline;
	if (device.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != vk::Result::eSuccess) {
		fmt::println("Failed to create pipeline");
		return VK_NULL_HANDLE;
	}
	else{
		return newPipeline;
	}
}

void PipelineBuilder::set_shaders(vk::ShaderModule vertexShader, vk::ShaderModule fragmentShader) {
	_shaderStages.clear();
	
	_shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eVertex, vertexShader));
	_shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eFragment, fragmentShader));
}

void PipelineBuilder::set_input_topology(vk::PrimitiveTopology topology) {
	_inputAssembly.topology = topology;
	_inputAssembly.primitiveRestartEnable = vk::False;
}

void PipelineBuilder::set_polygon_mode(vk::PolygonMode mode) {
	_rasterizer.polygonMode = mode;
	_rasterizer.lineWidth = 1.f; // not a lot of hardware supports hardware line rasterization of more than 1.f, sensible default
}

void PipelineBuilder::set_cull_mode(vk::CullModeFlags cullMode, vk::FrontFace frontFace) {
	_rasterizer.cullMode = cullMode;
	_rasterizer.frontFace = frontFace;
}

void PipelineBuilder::set_multisampling_none() {
	_multisampling.sampleShadingEnable = vk::False;
	// Multisampling defaultsed to none (1 sample per pixel)
	_multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	_multisampling.minSampleShading = 1.f;
	_multisampling.pSampleMask = nullptr;
	// No alpha to coverage either
	_multisampling.alphaToCoverageEnable = vk::False;
	_multisampling.alphaToOneEnable = vk::False;
}

void PipelineBuilder::disable_blending() {
	// Default write mask
	_colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	// No blending
	_colorBlendAttachment.blendEnable = vk::False;
}

void PipelineBuilder::enable_blending_additive() {
	_colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	_colorBlendAttachment.blendEnable = vk::True;
	_colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	_colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOne;
	_colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
	_colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	_colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	_colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
}

void PipelineBuilder::enable_blending_alphablend() {
	_colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	_colorBlendAttachment.blendEnable = vk::True;
	_colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	_colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	_colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
	_colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	_colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	_colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
}

void PipelineBuilder::set_color_attachment_format(vk::Format format) {
	_colorAttachmentFormat = format;
	// Connect the format to the renderInfo structure
	_renderInfo.colorAttachmentCount = 1;
	_renderInfo.pColorAttachmentFormats = &_colorAttachmentFormat;
}

void PipelineBuilder::set_depth_format(vk::Format format) {
	_renderInfo.depthAttachmentFormat = format;
}

void PipelineBuilder::disable_depthtest() {
	_depthStencil.depthTestEnable = vk::False;
	_depthStencil.depthWriteEnable = vk::False;
	_depthStencil.depthCompareOp = vk::CompareOp::eNever;
	_depthStencil.depthBoundsTestEnable = vk::False;
	_depthStencil.stencilTestEnable = vk::False;
	
	_depthStencil.minDepthBounds = 0.f;
	_depthStencil.maxDepthBounds = 1.f;
}

void PipelineBuilder::enable_depthtest(bool depthWriteEnable, vk::CompareOp op) {
	_depthStencil.depthTestEnable = vk::True;
	_depthStencil.depthWriteEnable = depthWriteEnable;
	_depthStencil.depthCompareOp = op;
	_depthStencil.depthBoundsTestEnable = vk::False;
	_depthStencil.stencilTestEnable = vk::False;

	_depthStencil.minDepthBounds = 0.f;
	_depthStencil.maxDepthBounds = 1.f;
}
//< pipelinebuilder

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