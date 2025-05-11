#pragma once

#include <cstdint>
#include <map>
#include <memory>

#include "Gpu.hpp"
#include "Resource.hpp"
#include "RenderPass.hpp"

namespace lft::rg {

struct RenderTask {
	std::shared_ptr<GpuTask> definition;

	VkRenderPass render_pass;
	std::vector<VkFramebuffer> framebuffer;

	VkExtent2D extent;
};

struct RenderGraphCommandBuffer {
	std::vector<VkCommandBuffer> command_buffers;
	std::vector<RenderTask> render_passes;

	VkSemaphore signal;
	std::vector<uint32_t> dependencies;
};

struct RenderGraphBuffer {
	std::vector<RenderGraphCommandBuffer> m_command_buffers;
	std::map<std::string, ImageResource> m_resources;

	VkFence m_fence;
};

class RenderGraph {
private:
	std::shared_ptr<Gpu> m_gpu;

	std::vector<RenderGraphBuffer> m_buffers;
	std::vector<GpuTask> m_task_definitions;
	uint32_t m_buffer_idx;

	void record_command_buffer(
			const RenderGraphCommandBuffer& command_buffer,
			uint32_t buffer_idx,
			uint32_t chain_image_idx
	);

	std::vector<VkSemaphoreSubmitInfoKHR> get_wait_semaphores_for(
			const RenderGraphBuffer& buffer,
			const RenderGraphCommandBuffer& block) const;

	void submit_command_buffer(
			const RenderGraphBuffer& buffer,
			const RenderGraphCommandBuffer& command_buffer,
			VkFence fence,
			uint32_t chain_image_idx);

	void wait_for_previous_frame(const RenderGraphBuffer& buffer);

public:
	VkSemaphore final_semaphore(uint32_t buffer_idx, uint32_t chain_image_idx) {
		return m_buffers[buffer_idx].m_command_buffers.back().signal;
	}

	RenderGraph(const std::shared_ptr<Gpu>& gpu,
				const std::vector<RenderGraphBuffer>& buffers) :
			m_gpu(gpu),
			m_buffers(std::move(buffers)), 
			m_buffer_idx(0) {

		VkFenceCreateInfo fence_info = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		VkFence fence;
		if(vkCreateFence(gpu->dev(), &fence_info, nullptr, &fence)) {
			throw std::runtime_error("Failed to create fence");
		}
	}

	void run(uint32_t chainImageIdx);
};

}
