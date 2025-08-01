#pragma once

#include <volk.h>

struct VertexBinding {
    uint32_t binding;
    uint32_t stride;
    VkVertexInputRate inputRate;

    VkVertexInputBindingDescription get_vk() {
        return *(VkVertexInputBindingDescription*)this;
    }
};
