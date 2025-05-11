#include "RenderGraph.hpp"
#include <algorithm>
#include <iostream>

namespace lft::rg {


void RenderGraph::wait_for_previous_frame(const RenderGraphBuffer& buffer) {
    vkWaitForFences(m_gpu->dev(),
        1,
        &buffer.m_fence,
        VK_TRUE, UINT64_MAX);
    vkResetFences(m_gpu->dev(),
        1,
        &buffer.m_fence);
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
		std::vector<VkClearValue> clear_values(rp.definition->outputs().size() + rp.definition->depth_output().has_value());
		for(uint32_t i = 0; i < rp.definition->outputs().size(); i++) {
			clear_values[i] = rp.definition->outputs()[i].clear_value();
		}

		if(rp.definition->depth_output().has_value()) {
			clear_values[rp.definition->outputs().size()] = rp.definition->depth_output()->clear_value();
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
			std::cout << "Wait: " << idx << std::endl;
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

	std::cout << "Submitting command buffer: " << command_buffer.render_passes[0].definition->name() << std::endl;

	auto wait_on_semaphores = get_wait_semaphores_for(buffer, command_buffer);

	for(auto& wait : wait_on_semaphores) {
		std::cout << "Waiting on semaphore: " << wait.semaphore << std::endl;
	}

	std::cout << "Signaling semaphore: " << command_buffer.signal << std::endl;

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

void RenderGraph::run(uint32_t chainImageIdx) {
	uint32_t buffer_idx = (m_buffer_idx + 1) % m_buffers.size();
	auto& buffer = m_buffers[buffer_idx];
	
	wait_for_previous_frame(buffer);

	uint32_t idx = 0;
	for(auto& cmdbuf : buffer.m_command_buffers) {
		idx++;
		record_command_buffer(cmdbuf, buffer_idx, chainImageIdx);

		if(idx == buffer.m_command_buffers.size()) {
			std::cout << std::format("Running last command buffer: ") << cmdbuf.signal << std::endl;
			submit_command_buffer(buffer, cmdbuf, buffer.m_fence, chainImageIdx);
		} else {
			submit_command_buffer(buffer, cmdbuf, VK_NULL_HANDLE, chainImageIdx);
		}
	}
}

}
