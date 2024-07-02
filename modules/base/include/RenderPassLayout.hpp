#pragma once

#include <volk/volk.h>

struct RenderPassLayout {
	VkRenderPass renderpass;
	VkDescriptorSetLayout setLayout[4];
	VkPipelineLayout layout;
};
