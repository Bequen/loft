#include "RenderGraph.hpp"
#include "AdjacencyMatrix.hpp"
#include "RenderPass.hpp"
#include <algorithm>
#include <iostream>
#include <print>

namespace lft::rg {

void RenderGraph::create_fences() {
	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	m_fences.resize(m_buffers.size());
	for(uint32_t fence_idx = 0;
		fence_idx < m_buffers.size();
		fence_idx++
	) {
		if(vkCreateFence(m_gpu->dev(),
			&fence_info, nullptr, &m_fences[fence_idx])) {
			throw std::runtime_error("Failed to create fence");
		}
	}
}

RenderGraph& RenderGraph::invalidate(const std::string& name) {
	/* for(auto& buffer : m_buffers) {
		for(uint32_t i = 0; i < buffer.m_command_buffers.size(); i++) {
			buffer.m_recording_validity[i] = false;
		}
		} */

	return *this;
}

RenderGraph::RenderGraph(
		const std::shared_ptr<Gpu>& gpu,
		const std::vector<RenderGraphBuffer*>& buffers,
		AdjacencyMatrix* dependencies
) :
	m_gpu(gpu),
	m_buffers(std::move(buffers)),
	m_buffer_idx(0),
	m_dependency_matrix(dependencies)
{
    if(m_buffers.size() == 0) {
        throw std::runtime_error("Number of buffers cannot be 0");
    }

	create_fences();
}

void RenderGraph::wait_for_previous_frame(uint32_t buffer_idx) {
    vkWaitForFences(m_gpu->dev(),
        1,
        &m_fences[buffer_idx],
        VK_TRUE, UINT64_MAX);
    vkResetFences(m_gpu->dev(),
        1,
        &m_fences[buffer_idx]);
}

void RenderGraph::record_command_buffer(
		uint32_t buffer_idx,
		uint32_t batch_idx,
		uint32_t output_idx
) {
    RenderGraphBuffer* pBuffer = m_buffers[buffer_idx];
	VkCommandBuffer cmdbuf = pBuffer->batch(batch_idx).output(output_idx).cmdbuf;

	VkCommandBufferBeginInfo cmdbuf_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};

	if(vkBeginCommandBuffer(cmdbuf,
				&cmdbuf_begin_info)) {
		throw std::runtime_error("Failed to begin command buffer");
	}

	TaskRecordInfo record_info(
		m_gpu,
		cmdbuf,
		buffer_idx,
		output_idx);

	auto& tasks = pBuffer->batch(batch_idx).tasks;
	for(auto& task : tasks) {
	    if(task.pDefinition.type() == GRAPHICS_TASK) {
    		std::vector<VkClearValue> clear_values(
    		        task.pDefinition.color_outputs().size() +
    				task.pDefinition.depth_output().has_value()
    		);
    		for(uint32_t i = 0; i < task.pDefinition.color_outputs().size(); i++) {
    			clear_values[i] = task.pDefinition.color_outputs()[i].clear_value();
    		}

    		if(task.pDefinition.depth_output().has_value()) {
    			clear_values[task.pDefinition.color_outputs().size()] =
    				task.pDefinition.depth_output()->clear_value();
    		}

    		VkRenderPassBeginInfo render_pass_begin_info = {
    			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    			.renderPass = task.render_pass.render_pass,
    			.framebuffer = task.framebuffer[output_idx],
    			.renderArea = {
    				.offset = {0, 0},
    				.extent = task.extent,
    			},
    			.clearValueCount = (uint32_t)clear_values.size(),
    			.pClearValues = clear_values.data(),
    		};

    		vkCmdBeginRenderPass(cmdbuf,
    				&render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		}

		task.pDefinition.m_record_func(record_info, task.pDefinition.m_pContext);

		if(task.pDefinition.type() == GRAPHICS_TASK) {
		    vkCmdEndRenderPass(cmdbuf);
		}
	}

	if(vkEndCommandBuffer(cmdbuf)) {
		throw std::runtime_error("Failed to end command buffer");
	}
}


std::vector<VkSemaphoreSubmitInfoKHR> RenderGraph::get_wait_semaphores_for(
		const RenderGraphBuffer* pBuffer,
		uint32_t batch_idx
) const {
	std::vector<VkSemaphoreSubmitInfoKHR> semaphores;

	for(auto& task : pBuffer->batch(batch_idx).tasks) {
	    auto dependencies = m_dependency_matrix->get_dependencies(task.pDefinition.name());

  		for(int32_t i = batch_idx - 1; i >= 0; i--) {
            for(auto dependency : dependencies) {
    		    if(std::find_if(pBuffer->batch(i).tasks.begin(), pBuffer->batch(i).tasks.end(),
				[dependency](const Task& other) {
				    return other.pDefinition.name() == dependency;
				}) != pBuffer->batch(i).tasks.end()) {
				    semaphores.push_back((VkSemaphoreSubmitInfoKHR) {
						.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
						.semaphore = pBuffer->batch(i).signal,
						.value = 1,
						.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
						.deviceIndex = 0
					});

					break;
                }
    		}
		}
	}

	return semaphores;
}

void RenderGraph::submit_command_buffer(
	uint32_t buffer_idx,
	uint32_t batch_idx,
	VkFence fence,
	uint32_t output_idx)
{
    RenderGraphBuffer* pBuffer = m_buffers[buffer_idx];

	VkCommandBufferSubmitInfoKHR cmdbuf = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
		.commandBuffer = pBuffer->batch(batch_idx).output(output_idx).cmdbuf,
		.deviceMask = 0,
	};

	VkSemaphoreSubmitInfo signal_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = batch_idx == pBuffer->num_batches() - 1 ?
		        pBuffer->final_signal(output_idx) :
				pBuffer->batch(batch_idx).signal,
			.value = 1,
			.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
			.deviceIndex = 0
	};

	auto wait_on_semaphores = get_wait_semaphores_for(pBuffer, batch_idx);

	VkSubmitInfo2 submit_info = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = (uint32_t)wait_on_semaphores.size(),
			.pWaitSemaphoreInfos = wait_on_semaphores.data(),
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &cmdbuf,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &signal_info,
	};

	m_gpu->enqueue_graphics(&submit_info, fence);
}

bool RenderGraph::is_recording_invalid(const RenderGraphBuffer& buffer,
		uint32_t cmdbuf_idx) {
	return false;
	// return !buffer.m_recording_validity[cmdbuf_idx];
}

void RenderGraph::run(uint32_t chainImageIdx) {
	uint32_t buffer_idx = (m_buffer_idx + 1) % m_buffers.size();
	auto& buffer = m_buffers[buffer_idx];

	wait_for_previous_frame(buffer_idx);

	for(uint32_t idx = 0; idx < buffer->num_batches(); idx++) {
		// if(is_recording_invalid(buffer, idx - 1)) {
			record_command_buffer(buffer_idx, idx, chainImageIdx);
		// }

		VkFence fence = idx == buffer->num_batches() - 1 ?
			m_fences[buffer_idx] : VK_NULL_HANDLE;
		submit_command_buffer(buffer_idx, idx, fence, chainImageIdx);
	}
}

}
