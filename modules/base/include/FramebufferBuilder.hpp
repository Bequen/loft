#pragma once

#include <volk/volk.h>
#include <vector>
#include "Gpu.hpp"

struct Framebuffer {
    VkFramebuffer framebuffer;

    Framebuffer(VkFramebuffer fb);

    Framebuffer(const Gpu *pGpu, const VkRenderPass renderpass, const VkExtent2D extent,
                const std::vector<VkImageView>& attachments);
};

class FramebufferBuilder {
private:
    VkRenderPass m_renderpass;
    VkExtent2D m_extent;

    std::vector<ImageView> m_attachments;

public:
    FramebufferBuilder(VkRenderPass renderpass, VkExtent2D extent);

    FramebufferBuilder(VkRenderPass renderpass, VkExtent2D extent, uint32_t numAttachments);

    FramebufferBuilder(VkRenderPass renderpass, VkExtent2D extent, std::vector<ImageView> attachments);

    FramebufferBuilder& set_attachment(uint32_t index, VkImageView view);

    Framebuffer build(Gpu *pGpu);
};
