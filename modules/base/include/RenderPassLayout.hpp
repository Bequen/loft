#pragma once

#include <vulkan/vulkan_core.h>

struct RenderPassLayout {
	VkRenderPass renderpass;
	VkDescriptorSetLayout setLayout[4];
	VkPipelineLayout layout;
};
