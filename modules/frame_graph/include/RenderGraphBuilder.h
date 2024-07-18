#pragma once

#include "RenderGraphNode.h"
#include "RenderPass.hpp"
#include "RenderGraph.hpp"
#include "AdjacencyMatrix.h"

#include <map>
#include <utility>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <volk/volk.h>

struct ExternalImageResource {
    ImageView view;
    VkSemaphore signal;

    ExternalImageResource(ImageView view, VkSemaphore signal) :
    view(view), signal(signal) {

    }
};

struct ImageResourceDependency {
    std::string name;

    std::vector<ExternalImageResource> resources;

    ImageResourceDependency(std::string name, std::vector<ExternalImageResource> resources) :
    name(std::move(name)), resources(std::move(resources)) {

    }
};

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
    std::string m_name;

    VkExtent2D m_extent;
    uint32_t m_numImagesInFlight;

    std::string m_outputName;

    std::vector<ImageResourceDependency> m_externalImageDependencies;

    VkRenderPass create_renderpass_for(const std::shared_ptr<const Gpu>& gpu, RenderGraphNode *pPass);

    /**
     * Build each renderpass that output into swapchain. Starting the tree. Then add those renderpasses to the root node.
     * @param pGpu GPU instance on which to build everything.
     * @param pCache Used for caching resources while building.
     * @param pSwapchainNode Root node.
     */
    std::vector<RenderGraphVkCommandBuffer>
    build_renderpasses(const std::shared_ptr<const Gpu>& gpu, RenderGraphBuilderCache *pCache);

    /*
     * Prepares single node
     * Marks the renderpass as visited.
     */
    int build_renderpass(const std::shared_ptr<const Gpu>& gpu, RenderGraphBuilderCache *pCache,
                         RenderGraphVkCommandBuffer *pCmdBuf, RenderGraphNode *pPass,
                         int depth, uint32_t renderpassIdx);

    std::vector<Framebuffer>
    collect_attachments(const std::shared_ptr<const Gpu>& gpu, VkRenderPass rp, RenderGraphBuilderCache *pCache, RenderGraphNode *pPass);

    VkSampler create_attachment_sampler(const std::shared_ptr<const Gpu>& gpu);

    std::vector<ImageView> get_attachment(const std::shared_ptr<const Gpu>& gpu, RenderGraphBuilderCache *pCache,
                                          uint32_t numImagesInFlight, VkExtent2D extent,
                                          bool isDepth, ImageResourceLayout* pLayout);

    Image create_image_from_layout(const std::shared_ptr<const Gpu>& gpu, VkExtent2D extent, ImageResourceLayout *pLayout, bool isDepth);

    Buffer create_buffer_from_layout(BufferResourceLayout *pLayout) {
        throw std::runtime_error("Not implemented");
    }

    void build_renderpass_dependencies(const std::shared_ptr<const Gpu>& gpu, RenderGraphBuilderCache *pCache,
                                       RenderGraphVkCommandBuffer *pCmdBuf, uint32_t renderpassIdx,
                                       int depth);

    std::vector<uint32_t>
    get_external_dependency_ids_for_renderpass(RenderGraphBuilderCache *pCache,
                                               RenderGraphVkCommandBuffer *pCmdBuf,
                                               uint32_t renderpassIdx);

public:
    GET(m_extent, extent);
    VkExtent2D extent_for(RenderPass* pPass) const;

    GET(m_renderpasses, renderpasses);
    GET(m_externalImageDependencies, external_dependencies);

    /**
     * Creates new render graph builder
     * @param extent
     */
    explicit RenderGraphBuilder(std::string name, std::string outputName, uint32_t numImageInFlight);

    RenderGraphBuilder& add_graphics_pass(RenderPass *pRenderPass) {
        m_renderpasses.emplace_back(pRenderPass);
        m_numRenderpasses++;
        return *this;
    }

    RenderGraphBuilder& add_external_image(const std::string& name,
                                           const std::vector<ExternalImageResource>& dependencies) {
        m_externalImageDependencies.emplace_back(name, dependencies);
        return *this;
    }

    /**
     * Builds a new render graph from builder for a specific output chain and given extent.
     * @param pGpu GPU on which the render graph will be executed
     * @param outputChain Specifies the target output destination where the render graph's final output will be presented.
     * @param extent Extent of the images in the render graph.
     * @return Returns runnable render graph.
     */
    RenderGraph build(const std::shared_ptr<const Gpu>& gpu, const ImageChain& outputChain, VkExtent2D extent);

    const AdjacencyMatrix
    build_adjacency_matrix() const;

    void print_dot() {

    }
};
