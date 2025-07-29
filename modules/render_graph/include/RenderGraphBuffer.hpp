#pragma once

#include <unordered_map>
#include <string>

#include "RenderPass.hpp"
#include "Resource.hpp"
#include "Task.hpp"

namespace lft::rg {

struct BatchOutput {
    VkCommandBuffer cmdbuf;
	bool is_recording_valid;

	BatchOutput(VkCommandBuffer cmdbuf);
};

struct Batch {
    std::vector<Task> tasks;
	std::vector<uint32_t> barriers;
	std::vector<BatchOutput> outputs;
	VkSemaphore signal;

	inline BatchOutput output(uint32_t idx) {
	    return outputs[idx];
	}

	Batch(std::vector<BatchOutput> outputs, VkSemaphore signal);

	Batch& invalidate_recordings();

	Batch& insert_task(uint32_t idx, Task& task);

	Batch& update_task(uint32_t idx, Task& task);

	Batch& remove_task(uint32_t idx);

	bool equals(const Batch& rhs) const;
};

class RenderGraphBuffer {
    std::shared_ptr<Gpu> m_gpu;
    uint32_t m_index;
public:
    std::unordered_map<std::string, BufferResource> m_buffer_resources;
	std::unordered_map<std::string, ImageResource> m_image_resources;
private:
	std::vector<Batch> m_batches;

	std::vector<VkSemaphore> m_final_semaphores;

public:
    GET(m_index, index);

    RenderGraphBuffer(
        std::shared_ptr<Gpu> gpu,
        uint32_t index,
        uint32_t num_outputs);

    Batch& batch(uint32_t idx) {
        return m_batches[idx];
    }

    const Batch& batch(uint32_t idx) const {
        return m_batches[idx];
    }

    uint32_t num_batches() const {
        return m_batches.size();
    }

    Batch& insert_batch(uint32_t idx, uint32_t num_outputs);

    void remove_batch(uint32_t idx);

    VkSemaphore final_signal(uint32_t output_idx) const {
        return m_final_semaphores[output_idx];
    }

#pragma region IMAGE RESOURCES

	bool has_image_resource(const std::string& name) const {
	    return m_image_resources.find(name) != m_image_resources.end();
	}

	std::optional<const ImageResource*>
	get_image_resource(const std::string& name) const {
	    if(!has_image_resource(name)) {
			return {};
		}

		return &m_image_resources.find(name)->second;
	}

	void put_image_resource(
	    const std::string& name,
	    const ImageResource& resource
	) {
        m_image_resources.insert({name, resource});
	}

#pragma endregion

	bool equals(const RenderGraphBuffer& other) const;
};

}
