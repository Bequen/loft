#pragma once

#include <cstdint>
#include <volk.h>

#include "Gpu.hpp"

class MipmapGenerator {
private:
    const Gpu* m_gpu;

    VkCommandBuffer m_commandBuffer;

    VkFence m_fence;

    VkFence create_fence(const Gpu* gpu);

    VkCommandBuffer create_command_buffer(const Gpu* gpu);

public:
    explicit MipmapGenerator(const Gpu* gpu);

    uint32_t generate(Image image, VkImageLayout oldLayout, VkExtent2D extent, VkImageSubresourceRange range);
};
