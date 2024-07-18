#include "RenderGraph.hpp"
#include "RenderPassBuildInfo.hpp"
#include "Gpu.hpp"

#include <volk/volk.h>
#include <queue>
#include <utility>

RenderGraph::RenderGraph(const std::shared_ptr<const Gpu>& gpu,
                         std::string name,
                         ImageChain outputChain,
                         std::vector<RenderGraphVkCommandBuffer> root,
                         uint32_t numFrames) :
	m_gpu(gpu),
    m_name(std::move(name)),
    m_outputChain(std::move(outputChain)),
    m_root(std::move(root)),
    m_frameIdx(0),
	m_numFrames(numFrames),
    m_frameFinished(numFrames) {

    for(uint32_t i = 0; i < numFrames; i++) {
        VkFenceCreateInfo semaphoreInfo = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        vkCreateFence(gpu->dev(), &semaphoreInfo, nullptr, &m_frameFinished[i]);
    }
}

std::vector<VkSemaphoreSubmitInfoKHR>
RenderGraph::run_dependencies(const RenderGraphVkCommandBuffer *pNode,
                              uint32_t imageIdx, uint32_t chainImageIdx) const {
    std::vector<VkSemaphoreSubmitInfoKHR> waitSemaphores(pNode->dependencies.size());
    uint32_t i = 0;
    for(auto& dependency : pNode->dependencies) {
        run_tree(dependency, imageIdx, chainImageIdx);
        waitSemaphores[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        waitSemaphores[i].semaphore = dependency->perImageSignal[imageIdx];
        waitSemaphores[i].value = 1;
        waitSemaphores[i].deviceIndex = 0;
        waitSemaphores[i].stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
        i++;
    }

    return waitSemaphores;
}

void record_node(RenderGraphVkCommandBuffer *pNode, uint32_t imageIdx, uint32_t framebufferIdx) {
    pNode->begin(imageIdx);

    for(auto& renderpass : pNode->renderpasses) {
        renderpass.begin(pNode->perImageCommandBuffer[imageIdx], renderpass.isGraphOutput ? framebufferIdx : imageIdx);

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
VkCommandBufferSubmitInfo RenderGraph::run_tree(const RenderGraphVkCommandBuffer *pNode, uint32_t imageIdx, uint32_t chainImageIdx) const {
    auto waitSemaphores = run_dependencies(pNode, imageIdx, chainImageIdx);

    record_node((RenderGraphVkCommandBuffer*)pNode, imageIdx, chainImageIdx);

    auto signal = pNode->perImageSignal[imageIdx];



    VkCommandBufferSubmitInfo commandBufferInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = pNode->perImageCommandBuffer[imageIdx],
            .deviceMask = 0
    };

    return commandBufferInfo;
}

VkSemaphore RenderGraph::run(uint32_t chainImageIdx) {
	m_frameIdx = (m_frameIdx++) % m_numFrames;

    vkWaitForFences(m_gpu->dev(), 1, &m_frameFinished[m_frameIdx], VK_TRUE, UINT64_MAX);
    vkResetFences(m_gpu->dev(), 1, &m_frameFinished[m_frameIdx]);

    std::vector<VkCommandBufferSubmitInfo> cmdbufs(m_root.size());
    std::vector<VkSemaphoreSubmitInfoKHR> waits;

    uint32_t i = 0;
    for(auto& dep : m_root) {
        cmdbufs[i++] = run_tree(&dep, m_frameIdx, chainImageIdx);
        for(auto& dep2 : dep.dependencies) {
            VkSemaphoreSubmitInfoKHR semaphore = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = dep2->perImageSignal[m_frameIdx],
                .value = 1,
                .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                .deviceIndex = 0,
            };
            waits.push_back(semaphore);
        }
    }

    VkSemaphoreSubmitInfo signalInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_root[0].perImageSignal[m_frameIdx],
            .value = 1,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
            .deviceIndex = 0
    };

    VkSubmitInfo2 submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = (uint32_t)waits.size(),
            .pWaitSemaphoreInfos = waits.data(),
            .commandBufferInfoCount = (uint32_t)cmdbufs.size(),
            .pCommandBufferInfos = cmdbufs.data(),
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signalInfo,
    };

    m_gpu->enqueue_graphics(&submitInfo, m_frameFinished[m_frameIdx]);

    return m_root[0].perImageSignal[m_frameIdx];
}
