#pragma once

#include "RenderGraphNode.h"
#include "RenderPass.hpp"
#include "RenderGraph.hpp"

#include <map>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

/**
 * Organizes entire rendering into a graph
 *
 * Advantages:
 *  - ResourceLayout virtualization
 *  - Automatic scheduling
 *  - Robustness (you can define everything once and it can be looked up in graph)
 */
class RenderGraphBuilder {
private:
    std::vector<RenderGraphNode> m_renderpasses;
    uint32_t m_numRenderpasses;

    RenderPass *m_pSwapchainPass;

    VkExtent2D m_extent;
    uint32_t m_numFrames;

    /*
     * Runs through all of renderpasses and returns those that contain output,
     * that pPass has as input.
     * @param pPass Dependencies of this RenderPass will be returned
     * @return vector of all the dependencies of pPass
     */
    std::vector<RenderGraphNode*> get_dependencies(RenderGraphNode *pPass);


    /**
     * Build each renderpass that output into swapchain. Starting the tree. Then add those renderpasses to the root node.
     * @param pGpu GPU instance on which to build everything.
     * @param pCache Used for caching resources while building.
     * @param pSwapchainNode Root node.
     */
    std::vector<RenderGraphVkCommandBuffer>
    build_swapchain_renderpass(Gpu *pGpu, RenderGraphBuilderCache *pCache, RenderGraphNode *pSwapchainNode);

    /*
     * Prepares single node
     * Marks the renderpass as visited.
     */
    int build_renderpass(Gpu *pGpu, RenderGraphBuilderCache *pCache,
                         RenderGraphVkCommandBuffer *pCmdBuf, RenderGraphNode *pPass,
                         int depth);

    std::vector<Framebuffer>
    collect_attachments(Gpu *pGpu, VkRenderPass rp, RenderGraphBuilderCache *pCache, RenderGraphNode *pPass);

    VkSampler create_attachment_sampler(Gpu *pGpu);

    std::vector<ImageView> get_attachment(Gpu *pGpu, RenderGraphBuilderCache *pCache,
                                          uint32_t numImagesInFlight, VkExtent2D extent,
                                          bool isDepth, ImageResourceLayout* pLayout);

    Image create_image_from_layout(Gpu *pGpu, VkExtent2D extent, ImageResourceLayout *pLayout, bool isDepth);

    Buffer create_buffer_from_layout(BufferResourceLayout *pLayout) {
        throw std::runtime_error("Not implemented");
    }

public:
    GET(m_numFrames, num_frames);

    GET(m_extent, extent);
    VkExtent2D extent_for(RenderPass* pPass) const;

    RenderGraphBuilder& set_num_frames(uint32_t value) {
        m_numFrames = value;
        return *this;
    }

    /**
     * Creates new render graph builder
     * @param extent
     */
    explicit RenderGraphBuilder(VkExtent2D extent);

    RenderGraphBuilder& add_graphics_pass(RenderPass *pRenderPass) {
        m_renderpasses.emplace_back(RenderGraphNode(pRenderPass));
        m_numRenderpasses++;
        return *this;
    }

    RenderGraphBuilder& add_compute_pass(RenderPass *pRenderPass) {
        return *this;
    }

    RenderGraph build(Gpu *pGpu, Swapchain *pSwapchain);

    void print_dot() {

    }
};
