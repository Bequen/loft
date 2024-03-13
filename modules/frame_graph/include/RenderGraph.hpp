#pragma once

#include <optional>
#include <vector>
#include <map>
#include <string>
#include <vulkan/vulkan_core.h>
#include <stdexcept>

#include "ResourceLayout.hpp"
#include "RenderPass.hpp"
#include "Resource.h"
#include "RenderGraphBuilderCache.h"
#include "RenderGraphNode.h"
#include "DebugUtils.h"
#include "ImageChain.h"


struct RenderGraphVkRenderPass {
    bool isGraphOutput;

    std::string name;
    float color[4];

    VkRenderPass renderpass;
    std::vector<VkClearValue> clearValues;
    VkExtent2D extent;
    std::vector<VkFramebuffer> framebuffers;

    RenderPass *pRenderPass;

    RenderGraphVkRenderPass(RenderPass *pRenderPass,
                            VkRenderPass renderpass,
                            VkExtent2D extent,
                            const std::vector<VkClearValue>& clearValues,
                            const std::vector<Framebuffer>& framebuffers,
                            bool isGraphOutput) :

                            renderpass(renderpass),
                            pRenderPass(pRenderPass),
                            extent(extent),
                            clearValues(clearValues),
                            framebuffers(framebuffers.size()),
                            isGraphOutput(isGraphOutput) {
        assert(renderpass != VK_NULL_HANDLE &&
               pRenderPass != nullptr);

        assert(extent.width != 0 && extent.height != 0);

        assert(clearValues.size() >= pRenderPass->num_outputs());

        assert(!framebuffers.empty());

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
        clearValues[0].color = {
                0.0f, 0.0f, 0.0f, 0.0f
        };

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

struct RenderGraphVkCommandBuffer {
    const Gpu *pGpu;

    std::vector<RenderGraphVkRenderPass> renderpasses;
    std::vector<VkSemaphore> perImageSignal;
    std::vector<VkCommandBuffer> perImageCommandBuffer;

    std::vector<RenderGraphVkCommandBuffer*> dependencies;

    std::vector<VkSemaphore> create_signals(Gpu *pGpu, uint32_t numImagesInFlight) {
        std::vector<VkSemaphore> signals(numImagesInFlight);

        VkSemaphoreCreateInfo semaphoreInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        for(uint32_t i = 0; i < numImagesInFlight; i++) {
            vkCreateSemaphore(pGpu->dev(), &semaphoreInfo, nullptr, &signals[i]);
        }

        return signals;
    }

    std::vector<VkCommandBuffer> create_command_buffers(Gpu *pGpu, uint32_t numImagesInFlight) {
        std::vector<VkCommandBuffer> commandBuffers(numImagesInFlight);

        VkCommandBufferAllocateInfo cmdBufInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = pGpu->graphics_command_pool(),
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = numImagesInFlight
        };

        if(vkAllocateCommandBuffers(pGpu->dev(), &cmdBufInfo, commandBuffers.data())) {
            throw std::runtime_error("Failed to create command buffer");
        }

        return commandBuffers;
    }

    RenderGraphVkCommandBuffer(Gpu *pGpu, uint32_t numImagesInFlight) :
        perImageCommandBuffer(create_command_buffers(pGpu, numImagesInFlight)),
        perImageSignal(create_signals(pGpu, numImagesInFlight)),
        pGpu(pGpu) {

    }

    void add_render_pass(const RenderGraphVkRenderPass& renderpass) {
        renderpasses.push_back(renderpass);
    }

    void add_dependency(RenderGraphVkCommandBuffer *pDependency) {
        dependencies.push_back(pDependency);
    }

    void begin(uint32_t imageIdx) {
        VkCommandBufferBeginInfo cmdBeginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };

        vkBeginCommandBuffer(perImageCommandBuffer[imageIdx], &cmdBeginInfo);
    }

    void end(uint32_t imageIdx) {
        vkEndCommandBuffer(perImageCommandBuffer[imageIdx]);
    }
};

class RenderGraph {
private:
	const Gpu *m_pGpu;

    const std::vector<RenderGraphVkCommandBuffer> m_root;

    const ImageChain m_outputChain;

	/* Information about frame index */
    uint32_t m_frameIdx;

	/* Exact number of images in flight that this RenderGraph was built from */
	const uint32_t m_numFrames;

    void run_swapchain_node(const RenderGraphVkCommandBuffer *pNode,
                            uint32_t bufferIdx, uint32_t chainImageIdx) const;

	void run_tree(const RenderGraphVkCommandBuffer *pNode,
                  uint32_t bufferIdx, uint32_t chainImageIdx) const;

public:
    GET(m_root, dependencies);

	RenderGraph(Gpu *pGpu,
                ImageChain outputChain,
                std::vector<RenderGraphVkCommandBuffer> root,
                uint32_t numFrames);
	
	VkSemaphore run(uint32_t chainImageIdx);

    std::vector<VkSemaphoreSubmitInfoKHR>
    run_dependencies(const RenderGraphVkCommandBuffer *pNode,
                     uint32_t imageIdx, uint32_t chainImageIdx) const;
};



