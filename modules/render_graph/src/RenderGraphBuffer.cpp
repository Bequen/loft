#include "RenderGraphBuffer.hpp"

#include <iterator>
#include <unordered_map>
#include <iostream>
#include <algorithm>

std::vector<VkCommandBuffer> allocate_cmdbufs(const Gpu* gpu, uint32_t count) {
    VkCommandBufferAllocateInfo cmdbuf_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = gpu->graphics_command_pool(),
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = count,
	};

	std::vector<VkCommandBuffer> cmdbufs(cmdbuf_info.commandBufferCount);
	if(vkAllocateCommandBuffers(gpu->dev(), &cmdbuf_info, cmdbufs.data())) {
		throw std::runtime_error("Failed to create command buffer");
	}

	return cmdbufs;
}


namespace lft::rg {

std::vector<BatchOutput> create_batch_outputs(const Gpu* gpu, uint32_t count) {
    auto cmdbufs = allocate_cmdbufs(gpu, count);
    std::vector<BatchOutput> outputs;

    std::transform(cmdbufs.begin(), cmdbufs.end(),
            std::back_inserter(outputs), [](VkCommandBuffer cmdbuf) {
                return BatchOutput(cmdbuf);
            });

    return outputs;
}

    BatchOutput::BatchOutput(VkCommandBuffer cmdbuf) :
        cmdbuf(cmdbuf),
        is_recording_valid(false) {

    }

    Batch::Batch(std::vector<BatchOutput> outputs, VkSemaphore signal) :
    outputs(outputs),
    signal(signal) {

    }

    Batch& Batch::invalidate_recordings() {
        for(auto& output : outputs) {
            output.is_recording_valid = false;
        }

        return *this;
    }

    Batch& Batch::insert_task(uint32_t idx, Task& task) {
        tasks.insert(tasks.begin() + idx, task);
        invalidate_recordings();
        return *this;
    }

    Batch& Batch::update_task(uint32_t idx, Task& task) {
        tasks[idx] = task;
        invalidate_recordings();
        return *this;
    }

    Batch& Batch::remove_task(uint32_t idx) {
        tasks.erase(tasks.begin() + idx);
        invalidate_recordings();
        return *this;
    }

    bool Batch::equals(const Batch& rhs) const {
        if(tasks.size() != rhs.tasks.size()) {
            return false;
        }

        for(uint32_t i = 0; i < tasks.size(); i++) {
            if(!tasks[i].equals(rhs.tasks[i])) {
                return false;
            }
        }

        if(barriers != rhs.barriers) {
            return false;
        }

        if(outputs.size() != rhs.outputs.size()) {
            return false;
        }

        for(uint32_t i = 0; i < outputs.size(); i++) {
            if(outputs[i].is_recording_valid != rhs.outputs[i].is_recording_valid) {
                return false;
            }
        }

        return true;
    }

    RenderGraphBuffer::RenderGraphBuffer(
        const Gpu* gpu,
        uint32_t index,
        uint32_t num_outputs
    ) : m_gpu(gpu), m_index(index), m_final_semaphores(num_outputs) {
        for(uint32_t i = 0; i < num_outputs; i++) {
            m_final_semaphores[i] = m_gpu->create_semaphore();
        }
    }

    Batch& RenderGraphBuffer::insert_batch(uint32_t idx, uint32_t num_outputs) {
        m_batches.insert(m_batches.begin() + idx,
            Batch(create_batch_outputs(m_gpu, num_outputs), m_gpu->create_semaphore()));
        return m_batches[idx];
    }

    void RenderGraphBuffer::remove_batch(uint32_t idx) {
        m_batches.erase(m_batches.begin() + idx);
    }

bool is_buffer_resources_equal(
    std::unordered_map<std::string, BufferResource> lhs,
    std::unordered_map<std::string, BufferResource> rhs
) {
    for(auto& value : rhs) {
        if(!lhs.contains(value.first)) {
            std::cout << "Missing buffer resource: " << value.first << std::endl;
            return false;
        }
    }

    return true;
}

bool is_image_resources_equal(
    std::unordered_map<std::string, ImageResource> lhs,
    std::unordered_map<std::string, ImageResource> rhs
) {
    for(auto& value : rhs) {
        if(!lhs.contains(value.first)) {
            std::cout << "Missing image resource: " << value.first << std::endl;
            return false;
        }
    }

    return true;
}

bool RenderGraphBuffer::equals(const RenderGraphBuffer& other) const {
    if(m_batches.size() != other.m_batches.size()) {
        std::cout << "Some batches are missing" << std::endl;
        return false;
    }

    for(uint32_t i = 0; i < m_batches.size(); i++) {
        if(!m_batches[i].equals(other.m_batches[i])) {
            std::cout << "Task queues are not the same" << std::endl;
            return false;
        }
    }

    // compare resources
    /* if(!is_buffer_resources_equal(m_buffer_resources, other.m_buffer_resources)) {
        return false;
        } */

    if(!is_image_resources_equal(m_image_resources, other.m_image_resources)) {
        return false;
    }

    return true;
}

}
