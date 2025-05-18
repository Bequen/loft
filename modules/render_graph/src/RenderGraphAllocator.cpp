
#include "RenderGraphAllocator.hpp"

#include <stdexcept>
#include <vector>

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#include "FramebufferBuilder.hpp"
#include "RenderPass.hpp"
#include "RenderGraph.hpp"

namespace lft::rg {

ImageResource Allocator::allocate_resource(
		const ImageResourceDefinition& definition, 
		bool is_color
) {
    MemoryAllocationInfo memory_info = {
            .usage = MEMORY_USAGE_AUTO_PREFER_DEVICE
    };

	ImageCreateInfo image_info = {
            .extent = definition.extent(),
            .format = definition.format(),
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                     (VkImageUsageFlags)(is_color ? 
							 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT :
							 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT),
            .aspectMask = (VkImageAspectFlags)(is_color ?
                          VK_IMAGE_ASPECT_COLOR_BIT :
                          VK_IMAGE_ASPECT_DEPTH_BIT),
            .arrayLayers = 1,
            .mipLevels = 1,
    };

	Image image = {};
	m_gpu->memory()->create_image(&image_info, &memory_info, &image);
	
	ImageView view = {};
	view = image.create_view(m_gpu, definition.format(), {
			.aspectMask = image_info.aspectMask,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
	});

	return ImageResource(image.img, view.view);
}

VkAttachmentDescription2 Allocator::create_attachment_description(
		const ImageResourceDefinition& definition, 
		bool is_color,
		std::map<std::string, uint32_t>& resource_count_down,
		std::set<std::string>& cleared_resources
) {
	VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
	VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout final_layout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImageLayout middle_layout = is_color ? 
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : 
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

	VkImageLayout last_layout = 
		(definition.name() == m_output_name ? m_output_chain.layout() :
		 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	final_layout = resource_count_down[definition.name()] > 1 ?
		middle_layout :
		last_layout;

	if(cleared_resources.find(definition.name()) != cleared_resources.end()/* context.is_clear(definition.name()) */) {
		load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
		initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	} else {
		cleared_resources.insert(definition.name());
		load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
		initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	// store op is dont care for the last one and store for every other
	VkAttachmentStoreOp store_op = resource_count_down[definition.name()] == 1 ?
		VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;


	// remember to count down the resource
	resource_count_down[definition.name()]--;

	return {
			.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
			.format = definition.format(),
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = load_op,
			.storeOp = store_op,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = initial_layout,
			.finalLayout = final_layout
	};
}

VkRenderPass Allocator::allocate_renderpass(
		const std::shared_ptr<GpuTask> task,
		std::map<std::string, uint32_t>& resource_count_down,
		std::set<std::string>& cleared_resources
) {
	size_t num_attachments = task->outputs().size() + task->depth_output().has_value();
	std::vector<VkAttachmentDescription2> descriptions(num_attachments);
    std::vector<VkAttachmentReference2> references(num_attachments);

	uint32_t i = 0;
	for(auto& output : task->outputs()) {
		descriptions[i] = create_attachment_description(
				output, true,
				resource_count_down,
				cleared_resources);
		
		// ignore sub passes, so one reference per attachment
        references[i] = {
                .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                .attachment = i,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
		i++;
	}

	if (task->depth_output().has_value()) {
		descriptions[i] = create_attachment_description(
				task->depth_output().value(),
				false,
				resource_count_down,
				cleared_resources);

        references[i] = {
                .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                .attachment = i,
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };
	}

	VkMemoryBarrier2KHR entryBarrier = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dstAccessMask = 0
    };

    const VkSubpassDependency2 subpass_dependencies[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
                    .pNext = &entryBarrier,
                    .srcSubpass = VK_SUBPASS_EXTERNAL,
                    .dstSubpass = 0,
                    .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
            }
    };

    const VkSubpassDescription2 subpass = {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = (uint32_t)references.size() - (task->depth_output().has_value()),
            .pColorAttachments = references.data(),
            .pDepthStencilAttachment = task->depth_output().has_value() ? &references[i] : nullptr,
    };

	VkRenderPassCreateInfo2 renderpass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
		
		.attachmentCount = (uint32_t)descriptions.size(),
		.pAttachments = descriptions.data(),

		.subpassCount = 1,
		.pSubpasses = &subpass,

		.dependencyCount = 1,
		.pDependencies = subpass_dependencies
	};

	VkRenderPass renderpass;
	if(vkCreateRenderPass2KHR(m_gpu->dev(), &renderpass_info, nullptr, &renderpass)) {
		throw std::runtime_error("Failed to create render pass");
	}

	return renderpass;
}

VkFramebuffer Allocator::create_framebuffer(
		const std::shared_ptr<GpuTask> task,
		VkRenderPass render_pass,
		uint32_t output_image_idx
) {
	std::vector<ImageView> attachments(task->outputs().size() + task->depth_output().has_value());
	uint32_t i = 0;
	for(auto& output : task->outputs()) {
		attachments[i++] = get_attachment(output.name(), output_image_idx).view;
	}

	if (task->depth_output().has_value()) {
		attachments[i++] = get_attachment(task->depth_output()->name(), output_image_idx).view;
	}

	if(i != attachments.size()) {
		throw std::runtime_error("Framebuffer attachments size mismatch");
	}

	auto fb = FramebufferBuilder(render_pass, task->extent(), attachments)
		.build(m_gpu);

	return fb.framebuffer;
}

std::vector<VkCommandBuffer> allocate_command_buffers(std::shared_ptr<Gpu> gpu, uint32_t count) {
	VkCommandBufferAllocateInfo cmdbuf_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = gpu->graphics_command_pool(),
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = count,
	};

	std::vector<VkCommandBuffer> cmd_bufs(count);
	if(vkAllocateCommandBuffers(gpu->dev(), &cmdbuf_info, cmd_bufs.data())) {
		throw std::runtime_error("Failed to create command buffer");
	}

	return std::move(cmd_bufs);
}

VkSemaphore create_semaphore(std::shared_ptr<Gpu> gpu) {
	VkSemaphoreCreateInfo semaphore_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	VkSemaphore semaphore;
	if(vkCreateSemaphore(gpu->dev(), &semaphore_info, nullptr, &semaphore)) {
		throw std::runtime_error("Failed to create semaphore");
	}

	return semaphore;
}

void Allocator::collect_resources(const std::vector<std::shared_ptr<GpuTask>>& tasks) {
	m_resources.resize(num_buffers());
	for(auto& task : tasks) {
		for(auto& output : task->outputs()) {
			if(m_resources[0].find(output.name()) == m_resources[0].end()) {
				for(uint32_t i = 0; i < num_buffers(); i++) {
					auto resource = allocate_resource(output, true);
					m_resources[i].insert({output.name(), resource});
				}
			}
		}

		if(task->depth_output().has_value()) {
			auto& depth_output = task->depth_output().value();
			if(m_resources[0].find(depth_output.name()) == m_resources[0].end()) {
				for(uint32_t i = 0; i < num_buffers(); i++) {
					auto resource = allocate_resource(depth_output, false);
					m_resources[i].insert({depth_output.name(), resource});
				}
			}
		}
	}
}

std::map<std::string, uint32_t> count_resource_writes(const std::vector<std::shared_ptr<GpuTask>>& tasks) {
	std::map<std::string, uint32_t> resource_count_down;
	for(auto& task : tasks) {
		for(auto& output : task->outputs()) {
			if(resource_count_down.find(output.name()) == resource_count_down.end()) {
				resource_count_down[output.name()] = 1;
			} else {
				resource_count_down[output.name()]++;
			}
		}

		if(task->depth_output().has_value()) {
			auto& depth_output = task->depth_output().value();
			if(resource_count_down.find(depth_output.name()) == resource_count_down.end()) {
				resource_count_down[depth_output.name()] = 1;
			} else {
				resource_count_down[depth_output.name()]++;
			}
		}
	}

	return resource_count_down;
}

void Allocator::prepare_renderpasses(const std::vector<std::shared_ptr<GpuTask>>& tasks) {
	std::map<std::string, uint32_t> resource_count_down = count_resource_writes(tasks);
	std::set<std::string> cleared_resources;

	// prepare render passes
	for(auto& task : tasks) {
		auto rp = allocate_renderpass(task, resource_count_down, cleared_resources);
		m_renderpasses.insert({task->name(), rp});
	}
}


RenderGraphBuffer Allocator::allocate_buffer(const GraphAllocationInfo& info, uint32_t buffer_idx) {
	int cmdbuf_idx = 0;
	std::vector<RenderGraphCommandBuffer> command_buffers;

	for(auto& cmdbuf : info.command_buffers) {
		std::vector<VkCommandBuffer> cmdbufs(info.output_chain.count());
		std::vector<RenderTask> render_tasks;
		
		for(uint32_t rp_idx = cmdbuf.first_renderpass_idx; 
			rp_idx < cmdbuf.first_renderpass_idx + cmdbuf.num_renderpasses; 
			rp_idx++
		) {
			std::vector<VkFramebuffer> framebuffers(info.output_chain.count());
			auto rp = info.render_passes[rp_idx];

			for(uint32_t image_idx = 0; image_idx < info.output_chain.count(); image_idx++) {
				auto fb = create_framebuffer(rp, m_renderpasses[rp->name()], image_idx);

				framebuffers[image_idx] = fb;
				cmdbufs[image_idx] = allocate_command_buffers(info.gpu, 1)[0];
			}

			render_tasks.push_back({
					.definition = rp,
					.render_pass = m_renderpasses[rp->name()],
					.framebuffer = framebuffers,
					.extent = info.output_chain.extent()
			});
		}

		command_buffers.push_back({
			.command_buffers = cmdbufs,
			.render_passes = render_tasks, 
			.signal = create_semaphore(info.gpu),
			.dependencies = cmdbuf.wait_signals_idx
		});
	}

	RenderGraphBuffer buffer(command_buffers, m_resources[buffer_idx]);

	return buffer;
}

}
