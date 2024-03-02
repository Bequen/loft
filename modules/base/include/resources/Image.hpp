#pragma once

#include <vulkan/vulkan_core.h>

#include "resources/GpuAllocation.h"
#include "ImageView.hpp"

class Gpu;

struct Image {
	VkImage img;
	GpuAllocation allocation;

    Image() : Image(VK_NULL_HANDLE, {}) {

    }

    Image(VkImage img, GpuAllocation allocation) :
    img(img), allocation(allocation) {

    }

	ImageView create_view(Gpu *pGpu, VkFormat format,
						  VkImageSubresourceRange subresource);
};
