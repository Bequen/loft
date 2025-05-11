#pragma once

#include <string>

#include "Gpu.hpp"

#include "RenderGraphDefinition.hpp"
#include "RenderPass.hpp"

namespace lft::rg {

struct RenderGraphResource {
};

struct RenderGraphRenderPassNode {
	VkRenderPass renderpass;
	VkFramebuffer framebuffer;

	RenderGraphRenderPassNode(
			VkRenderPass renderpass,
			VkFramebuffer framebuffer) : 
	renderpass(renderpass),
	framebuffer(framebuffer) {

	}
};

struct RenderGraphCommandBufferNode {
	VkCommandBuffer cmd_buf;

	// render pass queue
	std::vector<RenderGraphRenderPassNode> render_passes;

	std::vector<uint32_t> dependencies;
	uint32_t signal;
};

/**
 * Buffer for a frame in flight for render graph
 */
struct RenderGraphBuffer {
	// command buffer queue
	std::vector<RenderGraphCommandBufferNode> command_buffers;
	std::vector<RenderGraphCommandBufferNode> output_command_buffers;

	// resources in the buffer
	std::vector<RenderGraphResource> resources;
	std::vector<VkSemaphore> semaphores;

	VkFence frame_finished_fence;
};

/**
 * @brief RenderGraphVulkanRuntime is a runtime for the render graph
 */
class RenderGraphVulkanRuntime {
private:
	std::shared_ptr<Gpu> m_gpu;
	std::shared_ptr<ImageChain> m_output_chain;

	uint32_t m_buffer_idx;
	std::vector<RenderGraphBuffer> m_buffers;

	RenderGraphRenderPassNode create_render_pass(
		const RenderGraphDefinition& render_graph,
		const RenderPass& render_pass);

	std::optional<RenderGraphResource> get_resource(const std::string& name);

	void create_resource(const ImageResourceDefinition& definition);

	RenderGraphBuffer& get_next_buffer() {
		return m_buffers[m_buffer_idx++ % m_buffers.size()];
	}

	void wait_for_previous_run(RenderGraphBuffer& buffer) {
		vkWaitForFences(m_gpu->dev(),
				1,
				&buffer.frame_finished_fence,
				VK_TRUE, UINT64_MAX);

		vkResetFences(m_gpu->dev(),
				1,
				&buffer.frame_finished_fence);
	}

	void record_command_buffer(const RenderGraphCommandBufferNode& cmdbuf);

	void submit_command_buffer(const RenderGraphBuffer& buffer,
			const RenderGraphCommandBufferNode& cmdbuf);

public:
	RenderGraphVulkanRuntime(
			std::shared_ptr<Gpu> gpu,
			std::shared_ptr<ImageChain> output_chain,
			uint32_t num_buffers,
			RenderGraphDefinition& render_graph_definition
	);

	/**
	 * Invalidates single dependency in the render graph.
	 * This can cause either a rerun of a specific render pass and it's
	 * successors or a re-record of the command buffer.
	 */
	void invalidate(const std::string& name);

	/**
	 * Runs all the render passes that has invalid runtime dependencies
	 */
	void run(uint32_t chain_image_idx);
};

}
