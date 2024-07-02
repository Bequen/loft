#pragma once

#include "RenderPass.hpp"
#include <volk/volk.h>

#include <vector>
#include <cstdint>
#include <stdexcept>


/**
 * Internal wrapper around RenderPass, mainly to sign that is has been
 * visited.
 * TODO: Use bitset.
 */
struct RenderGraphNode {
private:
    RenderPass *pRenderPass;
    uint32_t flags;

    /* each edge is basically a semaphore */
    std::vector<RenderGraphNode*> m_dependencies;
    std::vector<VkSemaphore> m_perFrameSignal;

    std::vector<VkCommandBuffer> m_commandBuffers;

    std::vector<VkClearValue> m_clearValues;
    std::vector<Framebuffer> m_framebuffers;
    VkRenderPass m_renderpass;

    VkExtent2D m_extent;

public:

#pragma region PROPERTIES

    GET(m_extent, extent);

    /**
     * Gets semaphore that this renderpass will signal
     * @param frameIdx Which image in flight
     * @return Semaphore that will be signaled after this renderpass is done
     */
    [[nodiscard]] inline VkSemaphore signal_for_frame(uint32_t imageInFlightIdx) const {
        return m_perFrameSignal[imageInFlightIdx];
    }

    GET(m_renderpass, vk_renderpass);
    GET(pRenderPass, renderpass);
    REF(m_clearValues, clear_values);
    REF(m_dependencies, dependencies);

    inline RenderGraphNode& set_extent(VkExtent2D extent) {
        m_extent = extent;
        return *this;
    }

    inline RenderGraphNode& set_clear_values(const std::vector<VkClearValue>& clearValues) {
        m_clearValues = clearValues;
        return *this;
    }

#pragma endregion
    /**
     * TODO: Create command buffers in the object that manages scheduling, not
     * here (RenderGraph)
     */
    explicit RenderGraphNode(RenderPass *pRenderPass) :
            pRenderPass(pRenderPass), flags(0), m_renderpass(VK_NULL_HANDLE), m_extent({0, 0}) {
        m_clearValues.push_back({0.0f, 0.0f, 0.0f, 1.0f});
    }

    void create_semaphores(Gpu *pGpu, uint32_t numImagesInFlight) {
        VkSemaphoreCreateInfo semaphoreInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        m_perFrameSignal.resize(numImagesInFlight);
        for(uint32_t i = 0; i < numImagesInFlight; i++) {
            vkCreateSemaphore(pGpu->dev(), &semaphoreInfo, nullptr, &m_perFrameSignal[i]);
        }
    }

    void create_command_buffers(Gpu *pGpu, uint32_t numFrames) {
        m_commandBuffers.resize(numFrames);
        VkCommandBufferAllocateInfo cmdBufInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = pGpu->graphics_command_pool(),
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = numFrames
        };

        if(vkAllocateCommandBuffers(pGpu->dev(), &cmdBufInfo, m_commandBuffers.data())) {
            throw std::runtime_error("Failed to create command buffer");
        }
    }

    RenderGraphNode& add_dependency(RenderGraphNode *pDependency) {
        m_dependencies.emplace_back(pDependency);
        return *this;
    }

    [[nodiscard]] size_t num_dependencies() const {
        return m_dependencies.size();
    }

    [[nodiscard]] VkCommandBuffer command_buffer(uint32_t imageIdx) const {
        return m_commandBuffers[imageIdx % m_commandBuffers.size()];
    }

    [[nodiscard]] bool is_visited() const {
        return flags & (1 << 0);
    }

    RenderGraphNode& mark_visited() {
        flags |= (1 << 0);
        return *this;
    }

    RenderGraphNode& set_framebuffers(std::vector<Framebuffer>& framebuffers) {
        m_framebuffers = framebuffers;
        return *this;
    }

    [[nodiscard]] Framebuffer framebuffer(uint32_t idx) const {
        return m_framebuffers[idx];
    }

    RenderGraphNode& set_renderpass(VkRenderPass rp) {
        m_renderpass = rp;
        return *this;
    }


};