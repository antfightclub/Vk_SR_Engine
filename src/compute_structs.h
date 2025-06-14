#pragma once

#include "vk_types.h"

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

struct ComputeEffect {
	const char* name;
	vk::Pipeline pipeline;
	vk::PipelineLayout layout;

	ComputePushConstants data;
};