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
 * Builds a runnable render graph from a set of render passes.
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

    std::vector<std::string> m_oneTimeDependencies;

    VkRenderPass create_renderpass_for(const std::shared_ptr<const Gpu>& gpu, RenderGraphNode *pPass,
                                       std::optional<VkMemoryBarrier2KHR> exitBarrier);

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
                         int depth, uint32_t renderpassIdx, std::optional<VkMemoryBarrier2KHR> exitBarrier);

    std::vector<Framebuffer>
    collect_attachments(const std::shared_ptr<const Gpu>& gpu, VkRenderPass rp, RenderGraphBuilderCache *pCache, RenderGraphNode *pPass);

    VkSampler create_attachment_sampler(const std::shared_ptr<const Gpu>& gpu);

    std::vector<ImageView> get_attachment(const std::shared_ptr<const Gpu>& gpu, RenderGraphBuilderCache *pCache,
                                          uint32_t numImagesInFlight, VkExtent2D extent,
                                          bool isDepth, ImageResourceLayout* pLayout);

    Image create_image_from_layout(const std::shared_ptr<const Gpu>& gpu, VkExtent2D extent, ImageResourceLayout *pLayout, bool isDepth);

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

    /**
     * Adds graphics pass
     * @param pRenderPass Render pass to add as graphics pass
     * @return current instance
     */
    RenderGraphBuilder& add_graphics_pass(RenderPass *pRenderPass) {
        m_renderpasses.emplace_back(pRenderPass);
        m_numRenderpasses++;
        return *this;
    }

    /**
     * Adds external image resource to sync render graphs together
     * @param name Name of the resource
     * @param dependencies Dependency for each buffer
     * @return
     */
    RenderGraphBuilder& add_external_image(const std::string& name,
                                           const std::vector<ExternalImageResource>& dependencies) {
        m_externalImageDependencies.emplace_back(name, dependencies);
        return *this;
    }

    bool is_one_time_dependency(const std::optional<std::string>& name) const {
        if(!name.has_value()) return false;

        return std::find(m_oneTimeDependencies.begin(), m_oneTimeDependencies.end(), name) != m_oneTimeDependencies.end();
    }

    RenderGraphBuilder& invalidate_dependency_every_frame(const std::string& name) {
        m_oneTimeDependencies.push_back(name);
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
