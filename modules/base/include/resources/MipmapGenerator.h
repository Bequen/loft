#pragma once

#include <cstdint>
#include <volk/volk.h>

#include "Gpu.hpp"

class MipmapGenerator {
private:
    std::shared_ptr<const Gpu> m_gpu;

    VkCommandBuffer m_commandBuffer;

    VkFence m_fence;

    VkFence create_fence(const std::shared_ptr<const Gpu>& gpu);

    VkCommandBuffer create_command_buffer(const std::shared_ptr<const Gpu>& gpu);

public:
    explicit MipmapGenerator(const std::shared_ptr<const Gpu>& gpu);

    uint32_t generate(Image image, VkImageLayout oldLayout, VkExtent2D extent, VkImageSubresourceRange range);
};
