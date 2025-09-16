#include <algorithm>
#include <cstdio>
#include <format>
#include <ostream>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <cstring>

#include "RenderGraphBuilder.hpp"
#include "AdjacencyMatrix.hpp"
#include "FramebufferBuilder.hpp"
#include "RenderGraph.hpp"
#include "RenderGraphBuffer.hpp"
#include "RenderPass.hpp"
#include "RenderPassLayout.hpp"
#include "Resource.hpp"

namespace lft::rg {

std::vector<std::string> get_task_names(
		const std::vector<TaskInfo>& tasks
) {
    std::vector<std::string> names;
    std::transform(tasks.begin(), tasks.end(),
            names.begin(),
            [](const TaskInfo& i) {
                return i.name();
            });

	return names;
}


std::unordered_map<std::string, uint32_t> count_resource_image_writes(const std::vector<TaskInfo>& tasks) {
	std::unordered_map<std::string, uint32_t> resource_count_down;
	for(auto& task : tasks) {
		for(auto& color_output : task.color_outputs()) {
			if(resource_count_down.find(color_output.name()) == resource_count_down.end()) {
				resource_count_down[color_output.name()] = 1;
			} else {
				resource_count_down[color_output.name()]++;
			}
		}

		if(task.depth_output().has_value()) {
			auto& depth_output = task.depth_output().value();
			if(resource_count_down.find(depth_output.name()) == resource_count_down.end()) {
				resource_count_down[depth_output.name()] = 1;
			} else {
				resource_count_down[depth_output.name()]++;
			}
		}
	}

	return resource_count_down;
}

inline bool is_first_resource_write(
    std::unordered_set<std::string>& cleared_resources,
    const std::string& resource
) {
    return !cleared_resources.contains(resource);
}

inline bool is_last_resource_write(
    std::unordered_map<std::string, uint32_t>& resource_count_down,
    const std::string& resource
) {
    return resource_count_down[resource] == 1;
}

VkAttachmentDescription2 BuilderAllocator::create_attachment_description(
		const ImageResourceDescription& definition,
		bool is_first_write,
		bool is_last_write
) {

	VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
	VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout final_layout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImageLayout middle_layout = definition.is_color() ?
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

	VkImageLayout last_layout =
		(definition.name() == m_output_name ? m_output_chain.layout() :
		 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// TODO: If the write is not last yet outputs to the output chain, the layout gets overwriten by middle_layout anyway
	final_layout = is_last_write ?
	    (definition.name() == m_output_name ? m_output_chain.layout() : last_layout) :
		middle_layout;

	if(!is_first_write) {
		load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
		initial_layout = definition.is_color() ?
		    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
		    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	} else {
		load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
		initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	// store op is dont care for the last one and store for every other
	VkAttachmentStoreOp store_op = is_last_write && (!m_store_all_images) ?
		VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;

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


TaskRenderPass BuilderAllocator::allocate_renderpass(
		const TaskInfo& task,
		std::unordered_map<std::string, uint32_t>& resource_count_down,
		std::unordered_set<std::string>& cleared_resources
) {
	size_t num_attachments = task.color_outputs().size() + task.depth_output().has_value();
	std::vector<VkAttachmentDescription2> descriptions(num_attachments);
    std::vector<VkAttachmentReference2> references(num_attachments);

    TaskRenderPassState state(task.color_outputs().size(), task.depth_output().has_value());

	uint32_t i = 0;
	for(auto& output : task.color_outputs()) {
    	bool is_first_write = is_first_resource_write(cleared_resources, output.name());
        bool is_last_write = is_last_resource_write(resource_count_down, output.name());

        if(is_first_write) {
            state.set_resource_is_first(i);
        } if(is_last_write) {
            state.set_resource_is_last(i);
        }

		descriptions[i] = create_attachment_description(
				output,
				is_first_write,
				is_last_write
		);

		// ignore sub passes, so one reference per attachment
        references[i] = {
                .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                .attachment = i,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
		i++;
	}

	if (task.depth_output().has_value()) {
	    bool is_first_write = is_first_resource_write(cleared_resources, task.depth_output()->name());
        bool is_last_write = is_last_resource_write(resource_count_down, task.depth_output()->name());

        if(is_first_write) {
            state.set_resource_is_first(i);
        } if(is_last_write) {
            state.set_resource_is_last(i);
        }

		descriptions[i] = create_attachment_description(
				task.depth_output().value(),
				is_first_write, is_last_write);

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
            .colorAttachmentCount = (uint32_t)references.size() - (task.depth_output().has_value()),
            .pColorAttachments = references.data(),
            .pDepthStencilAttachment = task.depth_output().has_value() ? &references[i] : nullptr,
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
	VkDebugUtilsObjectNameInfoEXT render_pass_dbg_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_RENDER_PASS,
        .objectHandle = (uint64_t)renderpass,
        .pObjectName = strdup(std::format("[RP] {}", task.name()).c_str()),
    };
    vkSetDebugUtilsObjectNameEXT(m_gpu->dev(), &render_pass_dbg_info);

	return TaskRenderPass(renderpass, state);
}


ImageResource BuilderAllocator::allocate_image_resource(
    const ImageResourceDescription& desc
) const {
    MemoryAllocationInfo memory_info = {
            .usage = MEMORY_USAGE_AUTO_PREFER_DEVICE
    };

	ImageCreateInfo image_info = {
            .extent = desc.extent(),
            .format = desc.format(),
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                        (VkImageUsageFlags)(desc.is_color() ?
						 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT :
						 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT),
            .aspectMask = (VkImageAspectFlags)(desc.is_color() ?
                            VK_IMAGE_ASPECT_COLOR_BIT :
                            VK_IMAGE_ASPECT_DEPTH_BIT),
            .arrayLayers = 1,
            .mipLevels = 1,
    };

	Image image = {};
	m_gpu->memory()->create_image(&image_info, &memory_info, &image);

	ImageView view = {};
	view = image.create_view(m_gpu, desc.format(), {
			.aspectMask = image_info.aspectMask,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
	});

	return ImageResource(image.img, view.view);
}

BufferResource BuilderAllocator::allocate_buffer_resource(
    const BufferResourceDescription& desc
) const {

}

ImageResourceDescription BuilderAllocator::correct_resource_description(ImageResourceDescription desc) {
    // correct extent
    VkExtent2D extent = desc.extent();
    if(extent.width == 0) {
        extent.width = m_output_chain.extent().width;
    }

    if(extent.height == 0) {
        extent.height = m_output_chain.extent().height;
    }

    desc.set_extent(extent);
    return desc;
}

ImageView BuilderAllocator::get_attachment(
    const ImageResourceDescription& desc,
    RenderGraphBuffer* pBuffer,
    uint32_t output_idx
) {
    if(desc.name() == m_output_name) {
		return m_output_chain.views()[output_idx];
	}

	auto attachment = pBuffer->get_image_resource(desc.name());
	if(!attachment.has_value()) {
	    auto resource = allocate_image_resource(correct_resource_description(desc));

		VkDebugUtilsObjectNameInfoEXT img_dbg_info = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType = VK_OBJECT_TYPE_IMAGE,
            .objectHandle = (uint64_t)resource.image,
            .pObjectName = strdup(std::format("[IMG:buf({}):out({})]{}", pBuffer->index(), output_idx, desc.name()).c_str()),
        };
        vkSetDebugUtilsObjectNameEXT(m_gpu->dev(), &img_dbg_info);

        VkDebugUtilsObjectNameInfoEXT img_view_dbg_info = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType = VK_OBJECT_TYPE_IMAGE_VIEW,
            .objectHandle = (uint64_t)resource.image_view,
            .pObjectName = strdup(std::format("[IMG_VIEW:buf({}):out({})]{}", pBuffer->index(), output_idx, desc.name()).c_str()),
        };
        vkSetDebugUtilsObjectNameEXT(m_gpu->dev(), &img_view_dbg_info);

	    pBuffer->put_image_resource(desc.name(), resource);
		return resource.image_view;
	}

	return attachment.value()->image_view;
}

VkFramebuffer BuilderAllocator::create_framebuffer(
		const TaskInfo& task_info,
		VkRenderPass renderpass,
		RenderGraphBuffer *pBuffer,
		uint32_t output_idx
) {
	uint32_t num_attachments = task_info.color_outputs().size() +
		task_info.depth_output().has_value();
	std::vector<ImageView> attachments(num_attachments);
	uint32_t i = 0;
	for(auto& output : task_info.color_outputs()) {
		attachments[i++] = get_attachment(output, pBuffer, output_idx).view;
	}

	if (task_info.depth_output().has_value()) {
		attachments[i++] = get_attachment(task_info.depth_output().value(), pBuffer, output_idx);
	}

	if(i != attachments.size()) {
		throw std::runtime_error("Framebuffer attachments size mismatch");
	}

	auto fb = FramebufferBuilder(renderpass, task_info.m_extent, attachments)
		.build(m_gpu);
	VkDebugUtilsObjectNameInfoEXT render_pass_dbg_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_FRAMEBUFFER,
        .objectHandle = (uint64_t)fb.framebuffer,
        .pObjectName = strdup(std::format("[FB:buf({}):out({})]{}", pBuffer->index(), output_idx, task_info.name()).c_str()),
    };
    vkSetDebugUtilsObjectNameEXT(m_gpu->dev(), &render_pass_dbg_info);

	return fb.framebuffer;
}

Task BuilderAllocator::create_graphics_task(
    const TaskInfo& task_info,
    RenderGraphBuffer* pBuffer,
    TaskRenderPass render_pass
) {
    Task task = {.pDefinition = task_info, .render_pass = render_pass};

    // create framebuffer for each image in output chain
    std::vector<VkFramebuffer> framebuffers(m_output_chain.count());
	for(
	    uint32_t image_idx = 0;
		image_idx < m_output_chain.count();
		image_idx++
	) {
		auto fb = create_framebuffer(task_info, task.render_pass.render_pass, pBuffer, image_idx);
		framebuffers[image_idx] = fb;
	}

	task.framebuffer = framebuffers;
	task.extent = m_output_chain.extent();

	return task;
}

Task BuilderAllocator::create_compute_task(
    const TaskInfo& task_info,
    RenderGraphBuffer* pBuffer
) {
    Task task = {.pDefinition = task_info};

	return task;
}

Task BuilderAllocator::create_task(
    const TaskInfo& task_info,
    RenderGraphBuffer* pBuffer,
    std::unordered_set<std::string>& cleared_resources,
    std::unordered_map<std::string, uint32_t>& resource_count_down
) {
    if(task_info.type() == GRAPHICS_TASK) {
        auto render_pass = allocate_renderpass(task_info, resource_count_down, cleared_resources);
        return create_graphics_task(task_info, pBuffer, render_pass);
    } else if(task_info.type() == COMPUTE_TASK) {
        return create_compute_task(task_info, pBuffer);
    } else {
        throw std::runtime_error(std::format("Unknown task type for {}", task_info.name()));
    }
}

bool is_render_pass_updated(const Task& old_task,
    std::unordered_set<std::string> cleared_resources,
    std::unordered_map<std::string, uint32_t> resource_count_down
) {
    uint32_t i = 0;
    for(; i < old_task.pDefinition.m_color_outputs.size(); i++) {
        if(old_task.render_pass.state.is_resource_first(i) != is_first_resource_write(cleared_resources, old_task.pDefinition.m_color_outputs[i].name())) {
            return true;
        } else if(old_task.render_pass.state.is_resource_last(i) != is_last_resource_write(resource_count_down, old_task.pDefinition.m_color_outputs[i].name())) {
            return true;
        }
    }

    if(old_task.pDefinition.depth_output().has_value()) {
        if(old_task.render_pass.state.is_resource_first(i) != is_first_resource_write(cleared_resources, old_task.pDefinition.depth_output()->name())) {
            return true;
        } else if(old_task.render_pass.state.is_resource_last(i) != is_last_resource_write(resource_count_down, old_task.pDefinition.depth_output()->name())) {
            return true;
        }
    }

    return false;
}

void BuilderAllocator::update_task_buffer(const Task& task, const RenderGraphBuffer* pBuffer) {
    TaskBuildInfo task_build_info(
	    m_gpu,
		pBuffer->index(),
		num_buffers(),
	    get_viewport(),
		task.render_pass.render_pass,
		pBuffer->m_image_resources
	);
	task.pDefinition.build_func()(task_build_info, task.pDefinition.m_pContext);
}

void BuilderAllocator::update_task_queue(
    RenderGraphBuffer* pBuffer,
    const std::vector<TaskInfo>& task_infos
) {
    std::unordered_map<std::string, uint32_t> resource_count_down = count_resource_image_writes(task_infos);
	std::unordered_set<std::string> cleared_resources;

	uint32_t cmdbuf_idx = 0;

	int32_t remaining_tasks_in_cmdbuf = pBuffer->num_batches() == 0 ? 0 : pBuffer->batch(0).tasks.size();

    // lookahead method
    uint32_t next_task_idx = 0;
    uint32_t next_batch_idx = 0;

    uint32_t queue_idx = 0;

    while(next_batch_idx < pBuffer->num_batches()) {
        Batch* next_batch = &pBuffer->batch(next_batch_idx);
        Task* next_task = &next_batch->tasks[next_task_idx];

        if(queue_idx >= task_infos.size()) {
            next_batch->remove_task(next_task_idx);
            next_task_idx--;

            if(next_batch->tasks.size() == 0) {
                pBuffer->remove_batch(next_batch_idx);
                next_task_idx = 0;

                continue;
            }
        } else {
            // if next_task and next_queue_item does not match, we have to
            if(task_infos[queue_idx].name() != next_task->pDefinition.name()) {
                // if task that should be there is updated, it means it was inserted
                // if it would be updated, there would not be a name mismatch
                if(is_task_updated(task_infos[queue_idx].name())) {
                    auto task = create_task(task_infos[queue_idx], pBuffer, cleared_resources, resource_count_down);
                    // insert batch instead of new
         			pBuffer->insert_batch(next_batch_idx, m_output_chain.count())
                                // insert the new task
               					.insert_task(0, task);
                    // notify about update
         			update_task_buffer(task, pBuffer);

                    // now the next_task and task_infos[queue_idx] should match, let us move on to the next
                }
                // if task that should be there is not updated (it was already in the queue)
                // but task that is actually there is updated, we should remove the current task,
                // because the wanted task is next
                else if(is_task_updated(next_task->pDefinition.name())) {
                    next_batch->remove_task(next_task_idx);
                    if(next_batch->tasks.size() == 0) {
                        pBuffer->remove_batch(next_batch_idx);
                        continue;
                    }
                }
            }
            // if tasks match, but task is marked as updated, we should rebuild the task
            else if(is_task_updated(task_infos[queue_idx].name()) ||
                    is_render_pass_updated(*next_task, cleared_resources, resource_count_down)
            ) {
                auto task = create_task(task_infos[queue_idx], pBuffer, cleared_resources, resource_count_down);
    			next_batch->update_task(next_task_idx, task);
            }
        }

        for(auto& output : task_infos[queue_idx].color_outputs()) {
            resource_count_down[output.name()]--;
            cleared_resources.insert(output.name());
        }

        next_batch = &pBuffer->batch(next_batch_idx);

        next_task_idx++;
        if(next_task_idx == next_batch->tasks.size()) {
            next_task_idx = 0;
            next_batch_idx++;
        }
        queue_idx++;
    }

    for(; queue_idx < task_infos.size(); queue_idx++) {
	    auto task = create_task(task_infos[queue_idx], pBuffer, cleared_resources, resource_count_down);
		pBuffer->insert_batch(pBuffer->num_batches(), m_output_chain.count())
                .insert_task(0, task);
		update_task_buffer(task, pBuffer);

		for(auto& output : task_infos[queue_idx].color_outputs()) {
            resource_count_down[output.name()]--;
            cleared_resources.insert(output.name());
        }
	}
}

RenderGraph BuilderAllocator::allocate(
    std::vector<TaskInfo>& task_infos,
    AdjacencyMatrix *dependencies
) {
    for(TaskInfo& task_info : task_infos) {
        if(task_info.m_extent.width == 0.0f) {
            task_info.m_extent.width = m_output_chain.extent().width;
        }

        if(task_info.m_extent.height == 0.0f) {
            task_info.m_extent.height = m_output_chain.extent().height;
        }
    }

	std::vector<RenderGraphBuffer*> buffers(num_buffers());
	for(uint32_t buffer_idx = 0; buffer_idx < num_buffers(); buffer_idx++) {
	    update_task_queue(&m_buffers[buffer_idx], task_infos);
		buffers[buffer_idx] = &m_buffers[buffer_idx];
	}

	m_updated_tasks.clear();

	return RenderGraph(m_gpu, m_output_name, buffers, dependencies);
}

bool BuilderAllocator::equals(const BuilderAllocator& other) const {
    if(m_gpu.get() != other.m_gpu.get()) {
        std::cout << "GPU mismatch" << std::endl;
        return false;
    }

    if(m_output_name != other.m_output_name) {
        std::cout << "Output name mismatch" << std::endl;
        return false;
    }

    if(m_buffers.size() != other.m_buffers.size()) {
        std::cout << std::format("Buffer count mismatch: {} != {}", m_buffers.size(), other.m_buffers.size()) << std::endl;
        return false;
    }

    for(uint32_t i = 0; i < m_buffers.size(); i++) {
        if(!m_buffers[i].equals(other.m_buffers[i])) {
            std::cout << "Buffer mismatch at index " << i << std::endl;
            return false;
        }
    }

    if(!std::equal(m_updated_tasks.begin(), m_updated_tasks.end(),
        other.m_updated_tasks.begin(), other.m_updated_tasks.end())) {
        std::cout << "Updated tasks mismatch" << std::endl;
        return false;
    }

    return true;
}


#pragma region TOPOLOGY SORT


bool is_depending_on(const TaskInfo& task, const TaskInfo& depends_on) {
	return std::find_if(task.dependencies().begin(), task.dependencies().end(),
			[&](const auto& dependency) {
				if(dependency == depends_on.m_name) {
					return true;
				}

				if(depends_on.m_depth_output.has_value() &&
					dependency == depends_on.depth_output()->name()) {
					return true;
				}

				return (std::find_if(depends_on.color_outputs().begin(),
						depends_on.color_outputs().end(),
						[&](const auto& output) {
							return output.name() == dependency;
						})
					!= depends_on.color_outputs().end()) ||
					std::find_if(depends_on.buffer_outputs().begin(),
						depends_on.buffer_outputs().end(),
						[&](const auto& output) {
							return output.name() == dependency;
						}) != depends_on.buffer_outputs().end();
			}) != task.m_dependencies.end();
}

bool writes_to(const TaskInfo& task, const std::string& name) {
    if(std::find_if(task.m_color_outputs.begin(), task.m_color_outputs.end(),
        [name](const ImageResourceDescription& resource) {return resource.name() == name;}) != task.m_color_outputs.end()) {
        return true;
    }

    if(std::find_if(task.buffer_outputs().begin(), task.buffer_outputs().end(),
        [name](const BufferResourceDescription& resource) {return resource.name() == name;}) != task.buffer_outputs().end()) {
        return true;
    }

    return task.m_depth_output.has_value() && task.m_depth_output->name() == name;
}

std::vector<uint32_t> get_adjacent_idxs(
		std::vector<TaskInfo>* tasks,
		uint32_t adjacent_of
) {
	auto& task = (*tasks)[adjacent_of];

	std::vector<uint32_t> idxs;
	uint32_t idx = 0;
	for(auto dependency : *tasks) {
		if(dependency.name() == task.name()) {
			continue;
		}

		if(is_depending_on(dependency, task)) {
			idxs.push_back(idx);
		}

		idx++;
	}

	return idxs;
}

std::vector<std::string> collect_task_names(std::vector<TaskInfo>& tasks) {
    std::vector<std::string> names(tasks.size());
    std::transform(tasks.begin(), tasks.end(), names.begin(),
        [](const TaskInfo& item) { return item.name(); }
    );

    return names;
}

bool has_common_write(const TaskInfo& task1, const TaskInfo& task2) {
    for(auto& write : task1.buffer_outputs()) {
        for(auto& write2 : task2.buffer_outputs()) {
            if(write2.name() == write.name()) {
                return true;
            }
        }
    }

    if(task1.depth_output().has_value() && task2.depth_output().has_value() &&
        task1.depth_output()->name() == task2.depth_output()->name()) {
            return true;
        }

    for(auto& write : task1.color_outputs()) {
        for(auto& write2 : task2.color_outputs()) {
            if(write2.name() == write.name()) {
                return true;
            }
        }
    }

    return false;
}

AdjacencyMatrix* build_adj_matrix(std::vector<TaskInfo>& tasks, const std::string& output_name) {
    auto names = collect_task_names(tasks);
    names.push_back(output_name);
    AdjacencyMatrix* matrix = new AdjacencyMatrix(names);
    for(uint32_t y = 0; y < tasks.size(); y++) {
        for(uint32_t x = 0; x < tasks.size(); x++) {
            if(x == y) {
                continue;
            }

            if(is_depending_on(tasks[y], tasks[x])) {
                matrix->set(x, y);
            } else {
                if(has_common_write(tasks[y], tasks[x]) &&
                    matrix->get(y, x) == false &&
                    matrix->get(x, y) == false) {
                    matrix->set(y, x);
                }
            }
        }

        if(writes_to(tasks[y], output_name)) {
            matrix->set(y, tasks.size());
        }
    }

    matrix->transitive_reduction();

    return matrix;
}

std::vector<TaskInfo> topology_sort(std::vector<TaskInfo>& tasks, const std::string& output_name) {
    auto matrix = build_adj_matrix(tasks, output_name);

    // get final tasks
    std::queue<uint32_t> queue;
    std::vector<bool> done(tasks.size(), false);
    std::vector<TaskInfo> result;

    auto last = matrix->get_dependencies(tasks.size());
    for(auto& i : last) {
        queue.push(i);
    }

    while(!queue.empty()) {
        auto item = queue.front();
        queue.pop();

        if(done[item]) {
            continue;
        }

        result.push_back(tasks[item]);

        auto dependencies = matrix->get_dependencies(item);

        while(dependencies.size() == 1) {
            if(done[item]) break;

            done[item] = true;

            item = dependencies[0];
            dependencies = matrix->get_dependencies(item);
            result.push_back(tasks[item]);
        }

        for(auto& dependency : dependencies) {
            queue.push(dependency);
        }
    }

    std::reverse(result.begin(), result.end());
    return result;
}

#pragma endregion

RenderGraph Builder::build() {
	auto sorted_tasks = topology_sort(m_tasks, m_output_name);

	m_allocator.set_store_all_images(m_store_all_images);

	auto dependencies = build_adj_matrix(m_tasks, m_output_name);
	return m_allocator.allocate(sorted_tasks, dependencies);
}

}
