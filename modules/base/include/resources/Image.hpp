#pragma once

#include <string>

#include <volk/volk.h>

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

	ImageView create_view(Gpu *pGpu, VkFormat format,
						  VkImageSubresourceRange subresource);

    void set_debug_name(const Gpu *pGpu, const std::string& name) const;
};
