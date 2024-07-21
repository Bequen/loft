#pragma once

#include <string>

#include <volk/volk.h>
#include <memory>

#include "resources/GpuAllocation.h"
#include "ImageView.hpp"

class Gpu;

struct Image {
	VkImage img;
	GpuAllocation allocation;

public:
    Image() : Image(VK_NULL_HANDLE, {}) {

    }

    Image(VkImage img, GpuAllocation allocation) :
    img(img), allocation(allocation) {

    }

	ImageView create_view(const std::shared_ptr<const Gpu>& gpu, VkFormat format,
						  VkImageSubresourceRange subresource);

    void set_debug_name(const std::shared_ptr<const Gpu>& gpu, const std::string& name) const;
};
