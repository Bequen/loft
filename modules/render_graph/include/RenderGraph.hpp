#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <set>

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

	std::vector<std::string> recording_dependencies;
};

struct RenderGraphBuffer {
	std::vector<RenderGraphCommandBuffer> m_command_buffers;
	std::vector<bool> m_recording_validity;

	void invalidate_dependency(const std::string& dependency) {
		for(uint32_t i = 0; i < m_command_buffers.size(); i++) {
			auto& deps = m_command_buffers[i].recording_dependencies;
			if(std::find(deps.begin(), deps.end(), dependency) != deps.end()) {
				m_recording_validity[i] = false;
			}
		}
	}

	RenderGraphBuffer(std::vector<RenderGraphCommandBuffer> command_buffers,
			std::map<std::string, ImageResource> resources) :
		m_command_buffers(std::move(command_buffers)),
		m_recording_validity(m_command_buffers.size(), false) {
	}
};

/**
 * Lightweight definition of the render graph to be run.
 */
class RenderGraph {
private:
	std::shared_ptr<Gpu> m_gpu;

	std::vector<RenderGraphBuffer> m_buffers;
	std::vector<VkFence> m_fences;
	uint32_t m_buffer_idx;

	void create_fences();

	std::vector<VkSemaphoreSubmitInfoKHR> get_wait_semaphores_for(
			const RenderGraphBuffer& buffer,
			const RenderGraphCommandBuffer& block) const;

	void wait_for_previous_frame(uint32_t buffer_idx);

	bool is_recording_invalid(const RenderGraphBuffer& buffer, uint32_t cmdbuf_idx);

	void record_command_buffer(
			const RenderGraphCommandBuffer& command_buffer,
			uint32_t buffer_idx,
			uint32_t chain_image_idx
	);

	void submit_command_buffer(
			const RenderGraphBuffer& buffer,
			const RenderGraphCommandBuffer& command_buffer,
			VkFence fence,
			uint32_t chain_image_idx);


public:
	VkSemaphore final_semaphore(uint32_t buffer_idx, uint32_t chain_image_idx) {
		return m_buffers[buffer_idx].m_command_buffers.back().signal;
	}

	RenderGraph& invalidate(const std::string& name);

	RenderGraph(const std::shared_ptr<Gpu>& gpu,
				const std::vector<RenderGraphBuffer>& buffers);

	void run(uint32_t chainImageIdx);
};

}
