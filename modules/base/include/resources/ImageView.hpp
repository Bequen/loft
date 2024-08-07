#pragma once

#include <volk/volk.h>
#include <string>
#include <memory>

class Gpu;

struct ImageView {
	VkImageView view;

	ImageView() :
	view(VK_NULL_HANDLE) {
	}

	ImageView(VkImageView view) :
	view(view) {
	}

    void set_debug_name(const std::shared_ptr<const Gpu>& gpu, const std::string& name) const;
};
