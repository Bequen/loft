#pragma once

#include "Recording.hpp"

#include <volk.h>

namespace lft {

class ImageSubresource {
};

class ImageSubresourceRange {
    VkImageSubresourceRange m_range;

public:
    ImageSubresourceRange(const Image& image, 
            uint32_t base_mip_level, uint32_t level_count, 
            uint32_t base_array_layer, uint32_t layer_count) {
        m_range.aspectMask = image.m_aspect_mask;

        m_range.baseMipLevel = base_mip_level;
        m_range.levelCount = level_count;
        m_range.baseArrayLayer = base_array_layer;
        m_range.layerCount = layer_count;
    }

    static ImageSubresourceRange full(const Image& image) {
        return ImageSubresourceRange(image, 0, VK_REMAINING_MIP_LEVELS,
                0, VK_REMAINING_ARRAY_LAYERS);
    }

    static ImageSubresourceRange mipmap(const Image& image, uint32_t base_mip_level, uint32_t level_count) {
        return ImageSubresourceRange(image, 0, VK_REMAINING_MIP_LEVELS,
                0, VK_REMAINING_ARRAY_LAYERS);
    }


};


}
