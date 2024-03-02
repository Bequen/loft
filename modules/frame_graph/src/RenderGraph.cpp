#include "RenderGraph.hpp"
#include "RenderPass.hpp"
#include "RenderPassBuildInfo.hpp"
#include "Gpu.hpp"
#include "FramebufferBuilder.hpp"

#include <algorithm>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <queue>

RenderGraph::RenderGraph(Gpu *pGpu, Swapchain *pSwapchain, RenderGraphNode* root, uint32_t numFrames) :
	m_pGpu(pGpu), m_pSwapchain(pSwapchain), m_root(root), m_frameIdx(0),
	m_numFrames(numFrames), m_frames(numFrames) {

    prepare_frames();
}

void RenderGraph::record_node(const RenderGraphNode *pNode, VkCommandBuffer cmdbuf, Framebuffer framebuffer, uint32_t imageIdx) const {
    VkCommandBufferBeginInfo cmdBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(cmdbuf, &cmdBeginInfo);

    VkRenderPassBeginInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = pNode->vk_renderpass(),
            .framebuffer = framebuffer.framebuffer,
            .renderArea = {
                    .offset = { 0, 0 },
                    .extent = pNode->extent()
            },
            .clearValueCount = (uint32_t)pNode->clear_values().size(),
            .pClearValues = pNode->clear_values().data()
    };

    vkCmdBeginRenderPass(cmdbuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    pNode->renderpass()->record(RenderPassRecordInfo(cmdbuf, imageIdx));

    vkCmdEndRenderPass(cmdbuf);

    vkEndCommandBuffer(cmdbuf);
}

std::vector<VkSemaphore> RenderGraph::run_dependencies(const RenderGraphNode *pNode, const uint32_t imageIdx) const {
    std::vector<VkSemaphore> waitSemaphores(pNode->num_dependencies());
    uint32_t i = 0;
    for(auto& dependency : pNode->dependencies()) {
        run_tree(dependency, imageIdx);
        waitSemaphores[i++] = dependency->signal_for_frame(imageIdx);
    }

    return waitSemaphores;
}

/**
 * Records render graph node.
 * @param pNode
 * @param imageIdx
 * @param fence
 */
void RenderGraph::run_tree(const RenderGraphNode *pNode, const uint32_t imageIdx) const {
    auto waitSemaphores = run_dependencies(pNode, imageIdx);

    VkCommandBuffer cmdbuf = pNode->command_buffer(imageIdx);
    record_node(pNode, cmdbuf, pNode->framebuffer(imageIdx), imageIdx);

    auto signal = pNode->signal_for_frame(imageIdx);

    VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = (uint32_t)pNode->num_dependencies(),
            .pWaitSemaphores = waitSemaphores.data(),
            .pWaitDstStageMask = &stageFlags,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmdbuf,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &signal,
    };

    m_pGpu->enqueue_graphics(&submitInfo, VK_NULL_HANDLE);

}

void RenderGraph::run_swapchain_node(RenderGraphNode *pNode, const uint32_t imageIdx, const uint32_t swapchainImageIdx, VkFence fence) const {
    auto waitSemaphores = run_dependencies(pNode, imageIdx);

    VkCommandBuffer cmdbuf = pNode->command_buffer(imageIdx);
    record_node(pNode, cmdbuf, pNode->framebuffer(swapchainImageIdx), imageIdx);

    auto signal = pNode->signal_for_frame(imageIdx);

    VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = (uint32_t)pNode->num_dependencies(),
            .pWaitSemaphores = waitSemaphores.data(),
            .pWaitDstStageMask = &stageFlags,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmdbuf,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &signal
    };

    m_pGpu->enqueue_graphics(&submitInfo, fence);
}



void RenderGraph::run() {
	m_frameIdx = (m_frameIdx++) % m_numFrames;

	uint32_t imageIdx = 0;
	Frame *pFrame = &m_frames[m_frameIdx];
	
	vkWaitForFences(m_pGpu->dev(), 1, &pFrame->readyFence, VK_TRUE, UINT64_MAX);
	vkAcquireNextImageKHR(m_pGpu->dev(), m_pSwapchain->swapchain(), UINT64_MAX,
						  nullptr, pFrame->imageReadyFence, &imageIdx);

	vkWaitForFences(m_pGpu->dev(), 1, &pFrame->imageReadyFence, VK_TRUE,
					UINT64_MAX);
	vkResetFences(m_pGpu->dev(), 1, &pFrame->imageReadyFence);
	vkResetFences(m_pGpu->dev(), 1, &pFrame->readyFence);

    VkFence fence = pFrame->readyFence;
    VkSemaphore waitSemaphores[m_root->num_dependencies()];
    uint32_t i = 0;
    for(auto& dependency : m_root->dependencies()) {
        run_swapchain_node(dependency, m_frameIdx, imageIdx, fence);
        waitSemaphores[i++] = dependency->signal_for_frame(m_frameIdx);
        fence = VK_NULL_HANDLE;
    }

	VkSwapchainKHR swapchain[] = {m_pSwapchain->swapchain()};
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = (uint32_t)m_root->num_dependencies(),
		.pWaitSemaphores = waitSemaphores,
		.swapchainCount = 1,
		.pSwapchains = swapchain,
		.pImageIndices = &imageIdx
	};

	m_pGpu->enqueue_present(&presentInfo);

}

void RenderGraph::prepare_frames() {
    VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for(auto& frame : m_frames) {
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_pGpu->dev(), &fenceInfo, nullptr, &frame.readyFence);
        fenceInfo.flags = 0;
        vkCreateFence(m_pGpu->dev(), &fenceInfo, nullptr, &frame.imageReadyFence);
    }
}

