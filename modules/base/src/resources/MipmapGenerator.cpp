#include "resources/MipmapGenerator.h"
#include "Gpu.hpp"
#include "Recording.hpp"
#include "TransferTaskPipeline.hpp"
#include <vulkan/vulkan_core.h>

MipmapGenerator::MipmapGenerator(const Gpu* gpu) :
m_gpu(gpu), m_commandBuffer(create_command_buffer(gpu)), m_fence(create_fence(gpu)) {

}

VkFence MipmapGenerator::create_fence(const Gpu* gpu) {
    VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    VkFence fence = VK_NULL_HANDLE;
    vkCreateFence(gpu->dev(), &fenceInfo, nullptr, &fence);

    return fence;
}

VkCommandBuffer MipmapGenerator::create_command_buffer(const Gpu* gpu) {
    VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = gpu->transfer_command_pool(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
    };

    VkCommandBuffer cmdbuf = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(gpu->dev(), &allocInfo,
                             &cmdbuf);

    return cmdbuf;
}

uint32_t MipmapGenerator::generate(Image image, VkImageLayout oldLayout, VkExtent2D extent, VkImageSubresourceRange range) {
    vkWaitForFences(m_gpu->dev(), 1, &m_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_gpu->dev(), 1, &m_fence);


    VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(m_commandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout = oldLayout,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image.img,
            .subresourceRange = {
                    .aspectMask = range.aspectMask,
                    .baseMipLevel = 1,
                    .levelCount = VK_REMAINING_MIP_LEVELS,
                    .baseArrayLayer = 0,
                    .layerCount = range.layerCount,
            },
    };

    vkCmdPipelineBarrier(m_commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    barrier.subresourceRange.levelCount = 1;

    int32_t width = extent.width;
    auto layout = oldLayout;

    for (uint32_t i = 1; i < range.levelCount; i++) {
        uint32_t from_level = i - 1;
        uint32_t to_level = i;

        std::vector<VkImageMemoryBarrier> barriers(2, barrier);
        barriers[0].oldLayout = layout;
        barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barriers[0].subresourceRange.baseMipLevel = from_level;

        vkCmdPipelineBarrier(m_commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, barriers.data());

        VkImageBlit blit = {
                .srcSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = from_level,
                        .baseArrayLayer = 0,
                        .layerCount = range.layerCount,
                },
                .srcOffsets = {
                        { 0, 0, 0 },
                        { width, width, 1 }
                },
                .dstSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = to_level,
                        .baseArrayLayer = 0,
                        .layerCount = range.layerCount
                },
                .dstOffsets = {
                        { 0, 0, 0 },
                        {
                            width > 1 ? width / 2 : 1,
                            width > 1 ? width / 2 : 1, 
                            1
                        }
                },
        };

        vkCmdBlitImage(m_commandBuffer,
                       image.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit,
                       VK_FILTER_LINEAR);

        // transfer back to old layout
        /* barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barriers[0].newLayout = oldLayout;
        barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(m_commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, barriers.data()); */

        width /= 2;
        layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }

    std::vector<VkImageMemoryBarrier> barriers(2, barrier);

    barriers[0].subresourceRange.baseMipLevel = 0;
    barriers[0].subresourceRange.levelCount = range.levelCount - 1;

    barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].newLayout = oldLayout;
    barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;


    barriers[1].subresourceRange.baseMipLevel = range.levelCount - 1;
    barriers[1].subresourceRange.levelCount = 1;

    barriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].newLayout = oldLayout;
    barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(m_commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         2, barriers.data());


    vkEndCommandBuffer(m_commandBuffer);

    VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &m_commandBuffer
    };

    m_gpu->enqueue_transfer(&submitInfo, m_fence);

    return 0;
}
