#include "RenderGraph.hpp"
#include "RenderPassBuildInfo.hpp"
#include "Gpu.hpp"

#include <vulkan/vulkan_core.h>
#include <queue>
#include <utility>

RenderGraph::RenderGraph(Gpu *pGpu, std::string name, ImageChain outputChain,
                         std::vector<RenderGraphVkCommandBuffer> root, uint32_t numFrames) :
	m_pGpu(pGpu),
    m_name(std::move(name)),
    m_outputChain(std::move(outputChain)),
    m_root(std::move(root)),
    m_frameIdx(0),
	m_numFrames(numFrames) {

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
void RenderGraph::run_tree(const RenderGraphVkCommandBuffer *pNode, uint32_t imageIdx, uint32_t chainImageIdx) const {
    auto waitSemaphores = run_dependencies(pNode, imageIdx, chainImageIdx);

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

void RenderGraph::run_swapchain_node(const RenderGraphVkCommandBuffer *pNode,
                                     uint32_t imageIdx, uint32_t swapchainImageIdx) const {
    auto waitSemaphores = run_dependencies(pNode, imageIdx, swapchainImageIdx);

    for(auto& externalDependency : pNode->m_externalDependenciesIds) {
        if(m_externalDependencies[externalDependency].is_valid()) {
            waitSemaphores.push_back({
                                             .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                             .semaphore = m_externalDependencies[externalDependency].semaphore(),
                                             .value = 1,
                                             .stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
                                             .deviceIndex = 0
                                     });
        }
    }

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
            .waitSemaphoreInfoCount = (uint32_t)waitSemaphores.size(),
            .pWaitSemaphoreInfos = waitSemaphores.data(),
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferInfo,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signalInfo,
    };

    m_pGpu->enqueue_graphics(&submitInfo, VK_NULL_HANDLE);
}



VkSemaphore RenderGraph::run(uint32_t chainImageIdx) {
	m_frameIdx = (m_frameIdx++) % m_numFrames;

    for(auto& dep : m_root) {
        run_swapchain_node(&dep, m_frameIdx, chainImageIdx);
    }

    return m_root[0].perImageSignal[m_frameIdx];
}
