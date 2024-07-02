#pragma once

#include <cstdint>
#include <volk/volk.h>

struct VertexAttribute {
    uint32_t location;
    uint32_t binding;
    VkFormat format;
    uint32_t offset;

    VkVertexInputAttributeDescription get_vk() {
        return *(VkVertexInputAttributeDescription*)this;
    }
};
