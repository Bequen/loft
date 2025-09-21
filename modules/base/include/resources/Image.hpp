#pragma once

#include <string>

#include <volk.h>
#include <memory>

#include "resources/GpuAllocation.h"
#include "ImageView.hpp"

class Gpu;

struct Image {
	VkImage img;

    VkImageAspectFlagBits m_aspect_mask;
    uint32_t m_layer_count;
    uint32_t m_level_count;

	GpuAllocation allocation;

public:
    Image() : Image(VK_NULL_HANDLE, {}) {

    }

    Image(VkImage img, GpuAllocation allocation) :
    img(img), allocation(allocation) {

    }

	ImageView create_view(const Gpu* gpu, VkFormat format,
						  VkImageSubresourceRange subresource);

    void set_debug_name(const Gpu* gpu, const std::string& name) const;
};
