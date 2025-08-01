#pragma once

#include <volk.h>

struct RenderPassLayout {
	VkRenderPass renderpass;
	VkDescriptorSetLayout setLayout[4];
	VkPipelineLayout layout;
};
