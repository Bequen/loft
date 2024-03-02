#pragma once

#include <vulkan/vulkan_core.h>

struct ImageView {
	VkImageView view;

	ImageView() :
	view(VK_NULL_HANDLE) {
	}

	ImageView(VkImageView view) :
	view(view) {
	}
};
