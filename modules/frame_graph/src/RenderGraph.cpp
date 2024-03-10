#include "RenderGraph.hpp"
#include "RenderPass.hpp"
#include "RenderPassBuildInfo.hpp"
#include "Gpu.hpp"
#include "FramebufferBuilder.hpp"

#include <algorithm>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <queue>

RenderGraph::RenderGraph(Gpu *pGpu, Swapchain *pSwapchain, std::vector<RenderGraphVkCommandBuffer> root, uint32_t numFrames) :
	m_pGpu(pGpu), m_pSwapchain(pSwapchain), m_root(root), m_frameIdx(0),
	m_numFrames(numFrames), m_frames(numFrames) {

    prepare_frames();
}

std::vector<VkSemaphoreSubmitInfoKHR> RenderGraph::run_dependencies(const RenderGraphVkCommandBuffer *pNode, const uint32_t imageIdx) const {
    std::vector<VkSemaphoreSubmitInfoKHR> waitSemaphores(pNode->dependencies.size());
    uint32_t i = 0;
    for(auto& dependency : pNode->dependencies) {
        run_tree(dependency, imageIdx);
        waitSemaphores[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        waitSemaphores[i].semaphore = dependency->perImageSignal[imageIdx];
        waitSemaphores[i].value = 1;
        waitSemaphores[i].deviceIndex = 0;
        waitSemaphores[i].stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
        i++;
    }

    return waitSemaphores;
}

void record_node(RenderGraphVkCommandBuffer *pNode, const uint32_t imageIdx, const uint32_t framebufferIdx) {
    pNode->begin(imageIdx);

    for(auto& renderpass : pNode->renderpasses) {
        renderpass.begin(pNode->perImageCommandBuffer[imageIdx], framebufferIdx);

        auto recordInfo = RenderPassRecordInfo(pNode->perImageCommandBuffer[imageIdx], imageIdx);

        renderpass.pRenderPass->record(recordInfo);

        renderpass.end(pNode->perImageCommandBuffer[imageIdx]);
    }

    pNode->end(imageIdx);
}

/**
 * Records render graph node.
 * @param pNode
 * @param imageIdx
 * @param fence
 */
void RenderGraph::run_tree(const RenderGraphVkCommandBuffer *pNode, const uint32_t imageIdx) const {
    auto waitSemaphores = run_dependencies(pNode, imageIdx);

    record_node((RenderGraphVkCommandBuffer*)pNode, imageIdx, imageIdx);

    auto signal = pNode->perImageSignal[imageIdx];

    VkSemaphoreSubmitInfo signalInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = signal,
            .value = 1,
            .stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            .deviceIndex = 0
    };

    VkCommandBufferSubmitInfo commandBufferInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = pNode->perImageCommandBuffer[imageIdx],
            .deviceMask = 0
    };

    VkSubmitInfo2 submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = (uint32_t)pNode->dependencies.size(),
            .pWaitSemaphoreInfos = waitSemaphores.data(),
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferInfo,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signalInfo,
    };

    m_pGpu->enqueue_graphics(&submitInfo, VK_NULL_HANDLE);

}

void RenderGraph::run_swapchain_node(RenderGraphVkCommandBuffer *pNode, const uint32_t imageIdx, const uint32_t swapchainImageIdx, VkFence fence) const {
    auto waitSemaphores = run_dependencies(pNode, imageIdx);

    record_node((RenderGraphVkCommandBuffer*)pNode, imageIdx, swapchainImageIdx);

    auto signal = pNode->perImageSignal[imageIdx];

    VkSemaphoreSubmitInfo signalInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = signal,
            .value = 1,
            .stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            .deviceIndex = 0
    };

    VkCommandBufferSubmitInfo commandBufferInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = pNode->perImageCommandBuffer[imageIdx],
            .deviceMask = 0
    };

    VkSubmitInfo2 submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = (uint32_t)pNode->dependencies.size(),
            .pWaitSemaphoreInfos = waitSemaphores.data(),
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferInfo,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signalInfo,
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
    VkSemaphore semaphore = VK_NULL_HANDLE;

    for(auto& dep : m_root) {
        run_swapchain_node(&dep, m_frameIdx, imageIdx, fence);
        semaphore = dep.perImageSignal[m_frameIdx];
        fence = VK_NULL_HANDLE;
    }

	VkSwapchainKHR swapchain[] = {m_pSwapchain->swapchain()};
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &semaphore,
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

