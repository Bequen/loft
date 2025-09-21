#include <stdexcept>
#include "FramebufferBuilder.hpp"

Framebuffer::Framebuffer(VkFramebuffer framebuffer) :
framebuffer(framebuffer) {

}

Framebuffer::Framebuffer(const Gpu* gpu, const VkRenderPass renderpass, const VkExtent2D extent,
                         const std::vector<VkImageView>& attachments) {
    VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderpass,
            .attachmentCount = (uint32_t)attachments.size(),
            .pAttachments = attachments.data(),
            .width = extent.width,
            .height = extent.height,
            .layers = 1,
    };

    if(vkCreateFramebuffer(gpu->dev(), &framebufferInfo, nullptr, &framebuffer)) {
        throw std::runtime_error("Failed to create framebuffer");
    }
}

FramebufferBuilder::FramebufferBuilder(VkRenderPass renderpass, VkExtent2D extent) :
        FramebufferBuilder(renderpass, extent, 4) {

}

FramebufferBuilder::FramebufferBuilder(VkRenderPass renderpass, VkExtent2D extent, uint32_t numAttachments) :
m_renderpass(renderpass), m_extent(extent), m_attachments(numAttachments) {

}

FramebufferBuilder::FramebufferBuilder(VkRenderPass renderpass, VkExtent2D extent, std::vector<ImageView> attachments) :
        m_renderpass(renderpass), m_extent(extent), m_attachments(attachments) {

}

Framebuffer FramebufferBuilder::build(const Gpu* gpu) {
    VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = m_renderpass,
            .attachmentCount = (uint32_t)m_attachments.size(),
            .pAttachments = (VkImageView*)m_attachments.data(),
            .width = m_extent.width,
            .height = m_extent.height,
            .layers = 1,
    };

    VkFramebuffer framebuffer;
    if(vkCreateFramebuffer(gpu->dev(), &framebufferInfo, nullptr, &framebuffer)) {
        throw std::runtime_error("Failed to create framebuffer");
    }

    return {framebuffer};
}

FramebufferBuilder &FramebufferBuilder::set_attachment(uint32_t idx, VkImageView view) {
    m_attachments[idx] = ImageView(view);
    return *this;
}
