#pragma once

#include <utility>
#include <vector>
#include <optional>
#include <volk/volk.h>
#include <string>
#include <map>
#include "RenderGraphBuilderCache.h"
#include "FramebufferBuilder.hpp"

struct RenderPass;
class RenderGraph;
struct ImageResource;
struct Resource;

struct RenderPassRecordInfo {
private:

    VkCommandBuffer m_commandBuffer;
    uint32_t m_imageIdx;

public:
    RenderPassRecordInfo(VkCommandBuffer commandBuffer, uint32_t imageInFlightIdx) :
    m_commandBuffer(commandBuffer), m_imageIdx(imageInFlightIdx) {

    }

    inline VkCommandBuffer command_buffer() const {
        return m_commandBuffer;
    }

    inline uint32_t image_idx() const {
        return m_imageIdx;
    }
};

struct RenderPassInputInfoFrame {
    std::vector<Resource> resources;
};

struct RenderPassInputInfo {

};

struct RenderPassOutputInfo {
private:
    VkRenderPass m_renderpass;

    /**
     * Framebuffer for each image in flight
     */
    std::vector<Framebuffer> m_framebuffers;

    VkExtent2D m_extent;

public:
    RenderPassOutputInfo(VkRenderPass renderpass,
                         std::vector<Framebuffer> framebuffers,
                         VkExtent2D extent) :
            m_renderpass(renderpass),
            m_framebuffers(std::move(framebuffers)),
            m_extent(extent) {

    }

    [[nodiscard]] inline uint32_t num_framebuffers() const { return m_framebuffers.size(); }
    [[nodiscard]] inline const std::vector<Framebuffer>& framebuffers() const { return m_framebuffers; }

    [[nodiscard]] inline VkRenderPass renderpass() const { return m_renderpass; }

    [[nodiscard]] inline VkViewport viewport() const {
        VkViewport result = {};
        result.x = 0; result.y = 0;
        result.width = (float)m_extent.width;
        result.height = (float)m_extent.height;
        result.minDepth = 0.0f; result.maxDepth = 1.0f;
        return result;
    }
    
};

/**
 * RenderPass assigned properties from frame graph
 */
struct RenderPassBuildInfo {
private:
    Gpu *m_pGpu;

    RenderGraphBuilderCache *m_pCache;

    RenderPassOutputInfo m_output;

    VkSampler m_sampler;
    uint32_t m_numImagesInFlight;

public:
	RenderPassBuildInfo(Gpu *pGpu,
                        RenderGraphBuilderCache *pCache,
                        RenderPassOutputInfo output, VkSampler sampler,
                        uint32_t numImagesInFlight) :
        m_pGpu(pGpu),
		m_pCache(pCache),
        m_output(std::move(output)),
        m_sampler(sampler),
        m_numImagesInFlight(numImagesInFlight) {

	}

    [[nodiscard]] inline const RenderPassOutputInfo& output() const { return m_output; }
    [[nodiscard]] inline const Gpu* gpu() const { return m_pGpu; }
    [[nodiscard]] inline uint32_t num_images_in_flights() const { return m_numImagesInFlight; }

	std::vector<ImageResource*> collect_attachments(RenderPass *pRenderPass);

    ImageResource* get_image(const std::string& name, uint32_t idx) {
        return m_pCache->get_image(name, idx);
    }

    [[nodiscard]] inline VkSampler sampler() const {
        return m_sampler;
    }


};
