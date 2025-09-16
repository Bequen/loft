#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "AdjacencyMatrix.hpp"

#include "RenderGraph.hpp"
#include "ImageChain.hpp"
#include "RenderPass.hpp"
#include "RenderGraphAllocator.hpp"

namespace lft::rg {

class BuilderAllocator {
private:
	std::shared_ptr<Gpu> m_gpu;

	ImageChain m_output_chain;
	std::string m_output_name;

	std::vector<RenderGraphBuffer> m_buffers;

	std::unordered_set<std::string> m_updated_tasks;
	uint32_t m_num_buffers;

	bool is_task_updated(const std::string& name) {
		return std::find(m_updated_tasks.begin(),
				m_updated_tasks.end(),
				name) != m_updated_tasks.end();
	}

	Task create_graphics_task(
	    const TaskInfo& task_info,
	    RenderGraphBuffer* pBuffer,
		TaskRenderPass render_pass
	);

	Task create_compute_task(
	    const TaskInfo& task_info,
	    RenderGraphBuffer* pBuffer
	);

	Task create_task(
	    const TaskInfo& task_info,
		RenderGraphBuffer* pBuffer,
		std::unordered_set<std::string>& cleared_resources,
        std::unordered_map<std::string, uint32_t>& resource_count_down
	);

	ImageResource allocate_image_resource(
	    const ImageResourceDescription& desc
	) const;

	BufferResource allocate_buffer_resource(const BufferResourceDescription& desc) const;

    ImageResourceDescription correct_resource_description(ImageResourceDescription desc);

	/**
	 * Looks for an image resource in buffer at buffer_idx. Returns if found. Allocates if not found.
	 * If the wanted image resouce is in the output chain, the output_chain_idx is used.
	 */
	ImageView get_attachment(
	    const ImageResourceDescription& desc,
		RenderGraphBuffer* pBuffer,
		uint32_t output_idx
	);

	VkSemaphore create_semaphore() {
    	VkSemaphoreCreateInfo semaphore_info = {
    		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    	};

    	VkSemaphore semaphore;
    	if(vkCreateSemaphore(m_gpu->dev(), &semaphore_info, nullptr, &semaphore)) {
    		throw std::runtime_error("Failed to create semaphore");
    	}

    	return semaphore;
	}

	std::vector<VkCommandBuffer> allocate_command_buffer(uint32_t count) {
        VkCommandBufferAllocateInfo cmdbuf_info = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = m_gpu->graphics_command_pool(),
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = count,
		};

		std::vector<VkCommandBuffer> cmdbufs(cmdbuf_info.commandBufferCount);
		if(vkAllocateCommandBuffers(m_gpu->dev(), &cmdbuf_info, cmdbufs.data())) {
			throw std::runtime_error("Failed to create command buffer");
		}

		return cmdbufs;
	}

	void update_task_queue(
	    RenderGraphBuffer* pBuffer,
	    const std::vector<TaskInfo>& task_infos
	);

	VkViewport get_viewport() {
	    return (VkViewport) {
                .x = 0, .y = 0,
                .width = (float)m_output_chain.extent().width,
                .height = (float)m_output_chain.extent().height,
                .minDepth = 0, .maxDepth = 1.0
            };
	}

	void update_task_buffer(const Task& task, const RenderGraphBuffer* pBuffer);
	bool m_store_all_images = false;

public:
    void remove_task(const std::string& name) {
        m_updated_tasks.insert(name);
    }

    void set_store_all_images(bool value) {
        m_store_all_images = value;
    }

	GET(m_num_buffers, num_buffers);

	BuilderAllocator(std::shared_ptr<Gpu> gpu,
			ImageChain output_chain,
			const std::string& output_name,
			uint32_t num_buffers) :
		m_gpu(gpu),
		m_output_chain(output_chain),
		m_output_name(output_name),
		m_num_buffers(num_buffers)
	{
	    for(uint32_t i = 0; i < num_buffers; i++) {
	        m_buffers.emplace_back(m_gpu, i, m_output_chain.count());
		}
	}

	void mark_task_updated(const std::string& name) {
        m_updated_tasks.insert(name);
	}

	void add_buffer_resource(
	    const std::string& name,
	    const std::vector<Buffer>& buffers,
		size_t size
	) {
		/* if(buffers.size() <= m_output_chain.count()) {
			throw std::runtime_error("Buffer count must be greater than output chain count");
		} if(m_output_chain.count() % buffers.size() != 0) {
			throw std::runtime_error("Buffer count must be a multiple of output chain count");
		} */

		for(uint32_t i = 0; i < m_buffers.size(); i++) {
			m_buffers[i].m_buffer_resources.insert({name, BufferResource(buffers[i % buffers.size()].buf, size)});
		}
	}

	void add_image_resource(
	    const std::string& name,
		const std::vector<ImageResource> images
	) {
		if(images.size() <= m_output_chain.count()) {
			throw std::runtime_error("Resource count must be greater than output chain count");
		} if(m_output_chain.count() % images.size() != 0) {
			throw std::runtime_error("Resource count must be a multiple of output chain count");
		}

		for(uint32_t i = 0; i < m_buffers.size(); i++) {
			// m_buffers[i].m_image_resources[name] = images[i % images.size()];
		}
	}

	VkAttachmentDescription2 create_attachment_description(
			const ImageResourceDescription& definition,
			bool is_first_write,
			bool is_last_write
	);

	TaskRenderPass allocate_renderpass(
			const TaskInfo& task,
			std::unordered_map<std::string, uint32_t>& resource_count_down,
			std::unordered_set<std::string>& cleared_resources
	);

	VkFramebuffer create_framebuffer(
			const TaskInfo& task_info,
			VkRenderPass renderpass,
			RenderGraphBuffer* pBuffer,
			uint32_t output_idx
	);

	RenderGraph allocate(
	    std::vector<TaskInfo>& tasks,
    	AdjacencyMatrix *dependencies
	);

	bool equals(const BuilderAllocator& other) const;
};

std::vector<TaskInfo> topology_sort(std::vector<TaskInfo>& tasks, const std::string& output_name);

class Builder {
private:
	std::string m_output_name;
	std::map<std::string, uint32_t> m_name_to_task_idx;
	std::vector<TaskInfo> m_tasks;

	BuilderAllocator m_allocator;

	// counter for how many times a resource is written to
	std::unordered_map<std::string, uint32_t> m_resource_write_counts;

	const TaskInfo& get_task_by_name(const std::string& name) {
		if(m_name_to_task_idx.find(name) == m_name_to_task_idx.end()) {
			throw std::runtime_error("Task does not exist");
		}

		return m_tasks[m_name_to_task_idx[name]];
	}

	bool m_store_all_images;

public:
    void store_all_images() {
        m_store_all_images = true;
    }

	Builder(std::shared_ptr<Gpu> gpu,
			ImageChain output_chain,
			const std::string& output_name
	) :
		m_output_name(output_name),
		m_allocator(gpu, output_chain, output_name, 1)
	{
	}

	/**
	 * Adds allocated buffer resource
	 */
	void add_buffer_resource(
	    const std::string& name,
		const std::vector<Buffer>& buffers,
	    size_t size
	) {
		m_allocator.add_buffer_resource(name, buffers, size);
	}

	void add_image_resource(
	    const std::string& name,
		const std::vector<ImageResource> images
	) {
		m_allocator.add_image_resource(name, images);
	}

	bool is_task_ok(const TaskInfo& task) {
		for(auto& dependency : task.dependencies()) {
			for(auto& output : task.color_outputs()) {
				if(output.name() == dependency) {
					return false;
				}
			}

			if(task.depth_output().has_value() &&
				task.depth_output()->name() == dependency) {
				return false;
			}

			for(auto& output : task.buffer_outputs()) {
				if(output.name() == dependency) {
					return false;
				}
			}
		}

		return true;
	}

	void add_task(const TaskInfo& task) {
		if(!is_task_ok(task)) {
			throw std::runtime_error("Task " + task.name() + " output to one of it's dependencies. That is prohibited. To simulate this behaviour, for instance in compute shader, allocate the resource yourself and add it with `add_image_resource` or `add_buffer_resource`.");
		}

        std::string task_name = task.name();
		auto found = std::find_if(m_tasks.begin(), m_tasks.end(),
    	    [&task_name](const TaskInfo& i) {
    		    return i.name() == task_name;
    		});

		if(found != m_tasks.end()) {
		    m_tasks.erase(found);
		}

		m_tasks.push_back(task);
		m_name_to_task_idx[task.m_name] = m_tasks.size() - 1;
		m_allocator.mark_task_updated(task.name());
	}

	void remove_task(const std::string& name) {
        m_allocator.remove_task(name);
        m_tasks.erase(std::remove_if(m_tasks.begin(), m_tasks.end(),
            [name](const TaskInfo& task) {
                return task.name() == name;
            }), m_tasks.end());
	}

	RenderGraph build();
};

}
