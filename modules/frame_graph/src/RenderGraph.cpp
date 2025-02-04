#include "RenderGraph.hpp"
#include "RenderPassBuildInfo.hpp"
#include "Gpu.hpp"

#include <volk/volk.h>
#include <queue>
#include <utility>
#include <stack>

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
    m_frameFinished(numFrames, std::vector<VkFence>(m_root.size())) {

    for(uint32_t i = 0; i < numFrames; i++) {
        for(uint32_t o = 0; o < m_root.size(); o++) {
            VkFenceCreateInfo semaphoreInfo = {
                    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                    .flags = VK_FENCE_CREATE_SIGNALED_BIT
            };

            vkCreateFence(gpu->dev(), &semaphoreInfo, nullptr, &m_frameFinished[i][o]);
        }
    }
}


std::vector<VkSemaphoreSubmitInfoKHR>
RenderGraph::get_wait_semaphores_for(RenderGraphVkCommandBuffer& block, uint32_t imageIdx) {
    std::vector<VkSemaphoreSubmitInfoKHR> waitSemaphores(block.external_dependencies_ids().size() + block.dependencies().size());

    for(uint32_t i = 0; i < block.external_dependencies_ids().size(); i++) {
        waitSemaphores[i] = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_externalDependencies[block.external_dependencies_ids()[i]].m_semaphore,
            .value = 1,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
            .deviceIndex = 0,
        };
    }

    for(uint32_t i = 0; i < block.dependencies().size(); i++) {
        waitSemaphores[i] = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = block.dependencies()[i]->signal(imageIdx),
                .value = 1,
                .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                .deviceIndex = 0,
        };
    }

    return waitSemaphores;
}

void RenderGraph::run_dependencies_of(RenderGraphVkCommandBuffer& block, uint32_t chainImageIdx) {
    for(uint32_t i = 0; i < block.dependencies().size(); i++) {
        auto& dependency = (RenderGraphVkCommandBuffer&)*block.dependencies()[i];
        run_dependencies_of(dependency, chainImageIdx);

        if(!dependency.is_command_buffer_valid(dependency.is_chain_output() ? chainImageIdx : m_frameIdx)) {
            dependency.record(m_frameIdx, chainImageIdx);
        }

        auto waitSemaphores = get_wait_semaphores_for(dependency, m_frameIdx);

        VkCommandBufferSubmitInfoKHR cmdbuf = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
                .commandBuffer = dependency.command_buffer(dependency.is_chain_output() ? chainImageIdx : m_frameIdx),
                .deviceMask = 0,
        };

        VkSemaphoreSubmitInfo signalInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = dependency.signal(m_frameIdx),
                .value = 1,
                .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                .deviceIndex = 0
        };

        VkSubmitInfo2 submitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .waitSemaphoreInfoCount = (uint32_t)waitSemaphores.size(),
                .pWaitSemaphoreInfos = waitSemaphores.data(),
                .commandBufferInfoCount = 1,
                .pCommandBufferInfos = &cmdbuf,
                .signalSemaphoreInfoCount = 1,
                .pSignalSemaphoreInfos = &signalInfo,
        };

        m_gpu->enqueue_graphics(&submitInfo, VK_NULL_HANDLE);
    }
}

VkSemaphore RenderGraph::run(uint32_t chainImageIdx) {
	m_frameIdx = (m_frameIdx++) % m_numFrames;

    vkWaitForFences(m_gpu->dev(),
        m_frameFinished[m_frameIdx].size(),
        m_frameFinished[m_frameIdx].data(),
        VK_TRUE, UINT64_MAX);
    vkResetFences(m_gpu->dev(),
        m_frameFinished[m_frameIdx].size(),
        m_frameFinished[m_frameIdx].data());

    // run each root dependency
    for(uint32_t i = 0; i < m_root.size(); i++) {
        run_dependencies_of(m_root[i], chainImageIdx);

        if(!m_root[i].is_command_buffer_valid(chainImageIdx)) {
            m_root[i].record(m_frameIdx, chainImageIdx);
        }

        auto waitSemaphores = get_wait_semaphores_for(m_root[i], m_frameIdx);

        VkCommandBufferSubmitInfoKHR cmdbuf = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
            .commandBuffer = m_root[i].command_buffer(chainImageIdx),
            .deviceMask = 0,
        };

        VkSemaphoreSubmitInfo signalInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = m_root[i].signal(m_frameIdx),
                .value = 1,
                .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                .deviceIndex = 0
        };

        VkSubmitInfo2 submitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .waitSemaphoreInfoCount = (uint32_t)waitSemaphores.size(),
                .pWaitSemaphoreInfos = waitSemaphores.data(),
                .commandBufferInfoCount = 1,
                .pCommandBufferInfos = &cmdbuf,
                .signalSemaphoreInfoCount = 1,
                .pSignalSemaphoreInfos = &signalInfo,
        };

        m_gpu->enqueue_graphics(&submitInfo, m_frameFinished[m_frameIdx][i]);
    }

    m_invalidRecordingDependencies.clear();

    return m_root[0].signal(m_frameIdx);
}

void RenderGraph::invalidate(std::string dependency) {
    m_invalidRecordingDependencies.push_back(dependency);
}
