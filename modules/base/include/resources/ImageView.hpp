#pragma once

#include <volk/volk.h>
#include <string>

class Gpu;

struct ImageView {
	VkImageView view;

	ImageView() :
	view(VK_NULL_HANDLE) {
	}

	ImageView(VkImageView view) :
	view(view) {
	}

    void set_debug_name(const Gpu *pGpu, const std::string& name) const;
};
