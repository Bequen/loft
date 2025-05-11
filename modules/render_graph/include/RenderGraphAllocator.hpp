#include <set>
#include <map>
#include <string>
#include <iostream>
#include <volk/volk.h>

#include "ImageChain.hpp"
#include "RenderGraph.hpp"
#include "Resource.hpp"
#include "RenderPass.hpp"

namespace lft::rg {

struct CommandBufferDefinition {
	uint32_t first_renderpass_idx;
	uint32_t num_renderpasses;

	uint32_t signal_idx;
	std::vector<uint32_t> wait_signals_idx;
};

struct GraphAllocationInfo {
	std::shared_ptr<Gpu> gpu;
	ImageChain output_chain;
	std::string output_name;

	std::vector<ImageResourceDefinition> resources;

	std::vector<std::shared_ptr<GpuTask>> render_passes;
	std::vector<CommandBufferDefinition> command_buffers;
};

std::vector<VkCommandBuffer> allocate_command_buffers(std::shared_ptr<Gpu> gpu, uint32_t count);


class Allocator {
private:
	std::shared_ptr<Gpu> m_gpu;

	std::vector<std::map<std::string, ImageResource>> m_resources;
	std::map<std::string, uint32_t> m_resource_count_down;
	std::set<std::string> m_cleared_resources;

	const ImageChain& m_output_chain;
	std::string m_output_name;

	std::map<std::string, VkRenderPass> m_renderpasses;

	std::vector<RenderGraphBuffer> m_buffers;

	ImageResource allocate_resource(
			const ImageResourceDefinition& definition, 
			bool is_color
	);

	void collect_resources(const std::vector<std::shared_ptr<GpuTask>>& tasks);

	VkAttachmentDescription2 create_attachment_description(
			const ImageResourceDefinition& definition, 
			bool is_color
	);

	VkRenderPass allocate_renderpass(const std::shared_ptr<GpuTask> task);

	void prepare_renderpasses(const std::vector<std::shared_ptr<GpuTask>>& tasks);

	inline const ImageView get_attachment(
			const std::string& name, 
			uint32_t output_chain_idx
	) const {
		if(name == m_output_name) {
			return m_output_chain.views()[output_chain_idx];
		}

		if(m_resources[0].find(name) == m_resources[0].end()) {
			throw std::runtime_error("Resource " + name + " not found in context");
		}

		return ImageView(m_resources[0].find(name)->second.image_view);
	}

	VkFramebuffer create_framebuffer(
			const std::shared_ptr<GpuTask> task,
			VkRenderPass render_pass,
			uint32_t output_image_idx
	);

	RenderGraphBuffer allocate_buffer(const GraphAllocationInfo& info, uint32_t buffer_idx);

public:
	GET(m_resources.size(), num_buffers);

	Allocator(const GraphAllocationInfo& info, uint32_t num_buffers) :
		m_gpu(info.gpu),
		m_resources(num_buffers),
		m_output_chain(info.output_chain),
		m_output_name(info.output_name)
	{
		std::cout << "Output name: " << m_output_name << std::endl;

		collect_resources(info.render_passes);

		prepare_renderpasses(info.render_passes);

		for(uint32_t i = 0; i < num_buffers; i++) {
			m_buffers.push_back(allocate_buffer(info, i));
		}

		for(auto& renderpass : info.render_passes) {
			RenderPassBuildInfo build_info(info.gpu, num_buffers, {
					.x = 0,
					.y = 0,
					.width = (float)info.output_chain.extent().width,
					.height = (float)info.output_chain.extent().height,
					.minDepth = 0.0f,
					.maxDepth = 1.0f
				},
				m_renderpasses[renderpass->name()], m_resources);
			renderpass->build(build_info);
		}
	}

	RenderGraph allocate() {
		return RenderGraph(m_gpu, m_buffers);
	}

};

}
