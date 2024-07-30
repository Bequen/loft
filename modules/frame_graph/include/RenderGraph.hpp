#pragma once

#include <optional>
#include <utility>
#include <vector>
#include <map>
#include <string>
#include <volk/volk.h>
#include <stdexcept>

#include "ResourceLayout.hpp"
#include "RenderPass.hpp"
#include "Resource.h"
#include "RenderGraphBuilderCache.h"
#include "RenderGraphNode.h"
#include "DebugUtils.h"
#include "ImageChain.h"

/**
 * Groups together render passes into Vulkan render pass
 */
struct RenderGraphVkRenderPass {
    /* If the render pass outputs to output chain */
    bool isGraphOutput;

    /**
     * Debug properties
     */
    std::string name;
    float color[4];

    VkRenderPass renderpass;
    std::vector<VkClearValue> clearValues;
    VkExtent2D extent;
    std::vector<VkFramebuffer> framebuffers;

    RenderPass *pRenderPass;
    bool m_isChainOutput;

    RenderGraphVkRenderPass(std::string name,
                            RenderPass *pRenderPass,
                            VkRenderPass renderpass,
                            VkExtent2D extent,
                            const std::vector<VkClearValue>& clearValues,
                            const std::vector<Framebuffer>& framebuffers,
                            bool isGraphOutput) :

                            name(std::move(name)),
                            renderpass(renderpass),
                            pRenderPass(pRenderPass),
                            extent(extent),
                            clearValues(clearValues),
                            framebuffers(framebuffers.size()),
                            isGraphOutput(isGraphOutput) {
        ASSERT(renderpass != VK_NULL_HANDLE &&
               pRenderPass != nullptr);

        ASSERT(extent.width != 0 && extent.height != 0);

        ASSERT(clearValues.size() >= pRenderPass->num_outputs());

        ASSERT(!framebuffers.empty());

        color[0] = color[1] = color[2] = color[3] = 0.0f;

        for(uint32_t i = 0; i < framebuffers.size(); i++) {
            this->framebuffers[i] = framebuffers[i].framebuffer;
        }
    }

    /**
     * Allows to set color in debugger.
     * @return
     */
    RenderGraphVkRenderPass& set_debug_color(float r, float g, float b, float a) {
        color[0] = r;
        color[1] = g;
        color[2] = b;
        color[3] = a;
        return *this;
    }

    void begin(VkCommandBuffer cmdbuf, uint32_t imageIdx) {
        VkDebugUtilsLabelEXT labelInfo = {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
                .pLabelName = pRenderPass->name().c_str(),
                .color = { color[0], color[1], color[2], color[3] }
        };

        vkCmdBeginDebugUtilsLabel(cmdbuf, &labelInfo);


        VkRenderPassBeginInfo renderPassInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = renderpass,
                .framebuffer = framebuffers[imageIdx],
                .renderArea = {
                        .offset = { 0, 0 },
                        .extent = extent
                },
                .clearValueCount = (uint32_t)clearValues.size(),
                .pClearValues = clearValues.data()
        };

        vkCmdBeginRenderPass(cmdbuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void end(VkCommandBuffer cmdbuf) {
        vkCmdEndRenderPass(cmdbuf);

        vkCmdEndDebugUtilsLabel(cmdbuf);
    }
};

struct RenderGraphBarrierInfo {

};

struct RenderGraphVkCommandBufferBarrier {
    uint32_t index;
    RenderGraphBarrierInfo info;
};


/**
 * Represents a command buffer in the render graph
 */
struct RenderGraphVkCommandBuffer {
private:
    std::shared_ptr<const Gpu> m_gpu;

    std::vector<RenderGraphVkRenderPass> m_renderpasses;
    std::vector<RenderGraphVkCommandBufferBarrier> m_barriers;

    std::vector<VkSemaphore> perImageSignal;
    std::vector<VkCommandBuffer> perImageCommandBuffer;

    std::vector<RenderGraphVkCommandBuffer*> m_dependencies;

    std::vector<uint32_t> m_externalDependenciesIds;

    std::string m_recordingDependency;

    uint32_t m_numImages;

    bool m_isChainOutput;

    std::vector<bool> m_isCommandBufferValid;
    bool m_invalidateRecordingEveryFrame;

    void create_signals(uint32_t numImagesInFlight) {
        perImageSignal = std::vector<VkSemaphore>(numImagesInFlight);

        VkSemaphoreCreateInfo semaphoreInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        for(uint32_t i = 0; i < numImagesInFlight; i++) {
            vkCreateSemaphore(m_gpu->dev(), &semaphoreInfo, nullptr, &perImageSignal[i]);
        }
    }

public:
    REF(m_dependencies, dependencies);
    REF(m_externalDependenciesIds, external_dependencies_ids)
    REF(m_recordingDependency, recording_dependency);
    GET(m_numImages, num_output_images);
    REF(m_renderpasses, renderpasses);
    GET(m_isChainOutput, is_chain_output);

    inline const bool is_command_buffer_valid(uint32_t imageIdx) const {
        return m_isCommandBufferValid[imageIdx];
    }

    inline const VkCommandBuffer command_buffer(uint32_t imageIdx) const {
        return perImageCommandBuffer[imageIdx];
    }

    inline const VkSemaphore signal(uint32_t imageIdx) const {
        return perImageSignal[imageIdx];
    }

    RenderGraphVkCommandBuffer(const std::shared_ptr<const Gpu>& gpu, uint32_t numImagesInFlight, bool invalidateRecordingEveryFrame) :
        m_numImages(numImagesInFlight),
        m_gpu(gpu),
        m_isCommandBufferValid(numImagesInFlight, false),
        m_invalidateRecordingEveryFrame(invalidateRecordingEveryFrame) {
    }

    /**
     * Allocates number of buffers for the command buffer
     * @param numCommandBuffers number of buffers
     */
    void allocate_command_buffers(uint32_t numCommandBuffers) {
        perImageCommandBuffer = std::vector<VkCommandBuffer>(numCommandBuffers);

        VkCommandBufferAllocateInfo cmdBufInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = m_gpu->graphics_command_pool(),
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = numCommandBuffers
        };

        if(vkAllocateCommandBuffers(m_gpu->dev(), &cmdBufInfo, perImageCommandBuffer.data())) {
            throw std::runtime_error("Failed to create command buffer");
        }

        create_signals(numCommandBuffers);
    }

    inline void set_recording_dependency(const std::string& dependency) {
        m_recordingDependency = dependency;
    }

    inline void set_external_dependencies(const std::vector<uint32_t>& externalDependencies) {
        m_externalDependenciesIds = externalDependencies;
    }

    inline RenderGraphVkCommandBuffer& add_render_pass(const RenderGraphVkRenderPass& renderpass,
                                                       const std::optional<RenderGraphBarrierInfo> barrier) {
        m_renderpasses.push_back(renderpass);

        if(barrier.has_value()) {
            m_barriers.push_back({
                .index = (uint32_t)m_renderpasses.size() - 1,
                .info = barrier.value()
            });
        }

        m_isChainOutput = renderpass.isGraphOutput;

        return *this;
    }

    void add_dependency(RenderGraphVkCommandBuffer *pDependency) {
        m_dependencies.push_back(pDependency);
    }

    /**
     * Re-records the command buffer
     * @param imageIdx
     */
    void record(uint32_t imageIdx, uint32_t chainImageIdx) {
        VkCommandBufferBeginInfo cmdBeginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                //.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };

        if(m_invalidateRecordingEveryFrame) {
            cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        }

        uint32_t cmdbufIdx = m_isChainOutput ? chainImageIdx : imageIdx;
        auto cmdbuf = perImageCommandBuffer[cmdbufIdx];

        if(vkBeginCommandBuffer(cmdbuf, &cmdBeginInfo)) {
            throw std::runtime_error("Failed to begin command buffer");
        }

        int lastRenderpass = 0;
        /* for(auto & m_barrier : m_barriers) {
            for(int o = lastRenderpass; o < m_barrier.index; o++) {
                m_renderpasses[o].begin(perImageCommandBuffer[imageIdx], imageIdx);

                auto recordInfo = RenderPassRecordInfo(perImageCommandBuffer[imageIdx], imageIdx);
                m_renderpasses[o].pRenderPass->record(recordInfo);

                m_renderpasses[o].end(perImageCommandBuffer[imageIdx]);
            }
        } */

        for(uint32_t o = 0; o < m_renderpasses.size(); o++) {
            m_renderpasses[o].begin(cmdbuf, m_renderpasses[o].isGraphOutput ? chainImageIdx : imageIdx);

            auto recordInfo = RenderPassRecordInfo(cmdbuf, imageIdx);
            m_renderpasses[o].pRenderPass->record(recordInfo);

            m_renderpasses[o].end(cmdbuf);
        }


        if(vkEndCommandBuffer(cmdbuf)) {
            throw std::runtime_error("Failed to end command buffer");
        }

        m_isCommandBufferValid[cmdbufIdx] = !m_invalidateRecordingEveryFrame;
    }
};

struct RenderGraphExternalDependency {
    std::string m_name;
    VkSemaphore m_semaphore;

    RenderGraphExternalDependency(std::string name, VkSemaphore semaphore) :
    m_name(std::move(name)), m_semaphore(semaphore) {

    }

    inline bool is_valid() const {
        return m_semaphore != VK_NULL_HANDLE;
    }

    inline VkSemaphore semaphore() const {
        return m_semaphore;
    }

    inline void set_semaphore(VkSemaphore semaphore) {
        m_semaphore = semaphore;
    }

    inline std::string name() const {
        return m_name;
    }
};

/**
 * Runnable render graph structure.
 *
 */
class RenderGraph {
private:
    const std::string m_name;
	std::shared_ptr<const Gpu> m_gpu;
    const ImageChain m_outputChain;

    std::vector<RenderGraphVkCommandBuffer> m_root;

	const uint32_t m_numFrames;

    uint32_t m_frameIdx;

    std::vector<std::vector<VkFence>> m_frameFinished;

    std::vector<RenderGraphExternalDependency> m_externalDependencies;
    std::vector<std::string> m_invalidRecordingDependencies;

    std::vector<VkSemaphoreSubmitInfoKHR> get_wait_semaphores_for(RenderGraphVkCommandBuffer& block, uint32_t imageIdx);

    void run_dependencies_of(RenderGraphVkCommandBuffer& block, uint32_t chainImageIdx);

    void run_cmdbuf_block(RenderGraphVkCommandBuffer& node, uint32_t bufferIdx, uint32_t chainImageIdx);


public:
    GET(m_root, dependencies);
    GET(m_name, name);

    RenderGraph(const std::shared_ptr<const Gpu>& gpu,
                std::string name,
                ImageChain outputChain,
                std::vector<RenderGraphVkCommandBuffer> root,
                uint32_t numFrames);

    void set_external_dependency(const std::string& name, VkSemaphore semaphore) {
        for(auto& dependency : m_externalDependencies) {
            if(dependency.name() == name) {
                dependency.set_semaphore(semaphore);
                return;
            }
        }

        m_externalDependencies.emplace_back(name, semaphore);
    }

	VkSemaphore run(uint32_t chainImageIdx);

    void invalidate(std::string dependency);
};



