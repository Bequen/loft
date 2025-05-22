#include "RenderGraph.hpp"
#include <algorithm>

namespace lft::rg {

void RenderGraph::create_fences() {
	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	m_fences.resize(m_buffers.size());
	for(uint32_t fence_idx = 0;
		fence_idx < m_buffers.size(); 
		fence_idx++)
	{
		if(vkCreateFence(m_gpu->dev(), 
			&fence_info, nullptr, &m_fences[fence_idx])) {
			throw std::runtime_error("Failed to create fence");
		}
	}
}

RenderGraph& RenderGraph::invalidate(const std::string& name) {
	for(auto& buffer : m_buffers) {
		for(uint32_t i = 0; i < buffer.m_command_buffers.size(); i++) {
			buffer.m_recording_validity[i] = false;
		}
	}

	return *this;
}

RenderGraph::RenderGraph(
		const std::shared_ptr<Gpu>& gpu,
		const std::vector<RenderGraphBuffer>& buffers
) :
	m_gpu(gpu),
	m_buffers(std::move(buffers)), 
	m_buffer_idx(0) 
{
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
		const RenderGraphCommandBuffer& command_buffer,
		uint32_t buffer_idx,
		uint32_t chain_image_idx
) {
	VkCommandBuffer cmdbuf = command_buffer.command_buffers[chain_image_idx];

	VkCommandBufferBeginInfo cmdbuf_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};


	if(vkBeginCommandBuffer(cmdbuf,
				&cmdbuf_begin_info)) {
		throw std::runtime_error("Failed to begin command buffer");
	}

	RenderPassRecordInfo record_info(
		m_gpu,
		cmdbuf,
		buffer_idx,
		chain_image_idx);

	for(auto& rp : command_buffer.render_passes) {
		std::vector<VkClearValue> clear_values(rp.definition->outputs().size() +
				rp.definition->depth_output().has_value());
		for(uint32_t i = 0; i < rp.definition->outputs().size(); i++) {
			clear_values[i] = rp.definition->outputs()[i].clear_value();
		}

		if(rp.definition->depth_output().has_value()) {
			clear_values[rp.definition->outputs().size()] = 
				rp.definition->depth_output()->clear_value();
		}

		VkRenderPassBeginInfo render_pass_begin_info = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = rp.render_pass,
			.framebuffer = rp.framebuffer[chain_image_idx],
			.renderArea = {
				.offset = {0, 0},
				.extent = rp.extent,
			},
			.clearValueCount = (uint32_t)clear_values.size(),
			.pClearValues = clear_values.data(),
		};

		vkCmdBeginRenderPass(cmdbuf,
				&render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		rp.definition->record(record_info);

		vkCmdEndRenderPass(cmdbuf);
	}

	if(vkEndCommandBuffer(cmdbuf)) {
		throw std::runtime_error("Failed to end command buffer");
	}
}


std::vector<VkSemaphoreSubmitInfoKHR> RenderGraph::get_wait_semaphores_for(
		const RenderGraphBuffer& buffer,
		const RenderGraphCommandBuffer& block) const {
	std::vector<VkSemaphoreSubmitInfoKHR> semaphores;
	std::ranges::transform(block.dependencies, std::back_inserter(semaphores),
			[&buffer](uint32_t idx) {
				return VkSemaphoreSubmitInfoKHR{
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
					.semaphore = buffer.m_command_buffers[idx].signal,
					.value = 1,
					.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
					.deviceIndex = 0
				};
			});

	return semaphores;
}

void RenderGraph::submit_command_buffer(
	const RenderGraphBuffer& buffer,
	const RenderGraphCommandBuffer& command_buffer,
	VkFence fence,
	uint32_t chain_image_idx) 
{
	VkCommandBufferSubmitInfoKHR cmdbuf = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
		.commandBuffer = command_buffer.command_buffers[chain_image_idx],
		.deviceMask = 0,
	};

	VkSemaphoreSubmitInfo signal_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = command_buffer.signal,
			.value = 1,
			.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
			.deviceIndex = 0
	};

	auto wait_on_semaphores = get_wait_semaphores_for(buffer, command_buffer);

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
	return !buffer.m_recording_validity[cmdbuf_idx];
}

void RenderGraph::run(uint32_t chainImageIdx) {
	uint32_t buffer_idx = (m_buffer_idx + 1) % m_buffers.size();
	auto& buffer = m_buffers[buffer_idx];
	
	wait_for_previous_frame(buffer_idx);

	uint32_t idx = 0;
	for(auto& cmdbuf : buffer.m_command_buffers) {
		idx++;
		if(is_recording_invalid(buffer, idx - 1)) {
			record_command_buffer(cmdbuf, buffer_idx, chainImageIdx);
		}

		VkFence fence = idx == buffer.m_command_buffers.size() ? 
			m_fences[buffer_idx] : VK_NULL_HANDLE;
		submit_command_buffer(buffer, cmdbuf, fence, chainImageIdx);
	}
}

}
