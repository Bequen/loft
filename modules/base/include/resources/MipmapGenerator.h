#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>

#include "Gpu.hpp"

class MipmapGenerator {
private:
    Gpu *m_pGpu;

    VkCommandBuffer m_commandBuffer;

    VkFence m_fence;

    VkFence create_fence(Gpu *pGpu);

    VkCommandBuffer create_command_buffer(Gpu *pGpu);

public:
    MipmapGenerator(Gpu *pGpu);

    uint32_t generate(Image image, VkImageLayout oldLayout, VkExtent2D extent, VkImageSubresourceRange range);
};
