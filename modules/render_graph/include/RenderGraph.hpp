#pragma once

#include <cstdint>
#include <map>
#include <memory>

#include "AdjacencyMatrix.hpp"
#include "Gpu.hpp"
#include "RenderGraphBuffer.hpp"

namespace lft::rg {

/**
 * Lightweight definition of the render graph to be run.
 */
class RenderGraph {
private:
	std::shared_ptr<Gpu> m_gpu;

	AdjacencyMatrix* m_dependency_matrix;
	std::vector<RenderGraphBuffer*> m_buffers;
	std::vector<VkFence> m_fences;
	uint32_t m_buffer_idx;

	void create_fences();

	std::vector<VkSemaphoreSubmitInfoKHR> get_wait_semaphores_for(
	        const RenderGraphBuffer* pBuffer,
			uint32_t cmdbuf_idx
	) const;

	void wait_for_previous_frame(uint32_t buffer_idx);

	bool is_recording_invalid(const RenderGraphBuffer& buffer, uint32_t cmdbuf_idx);

	void record_command_buffer(
            uint32_t buffer_idx,
           	uint32_t cmdbuf_idx,
            uint32_t output_idx
	);

	void submit_command_buffer(
			uint32_t buffer_idx,
			uint32_t cmdbuf_idx,
			VkFence fence,
			uint32_t output_idx
	);


public:
	const RenderGraphBuffer& buffer(uint32_t idx) const {
	    return *m_buffers[idx];
	}

	RenderGraph& invalidate(const std::string& name);

	RenderGraph(const std::shared_ptr<Gpu>& gpu,
	            const std::vector<RenderGraphBuffer*>& buffers,
				AdjacencyMatrix* dependencies
    );

    void run(uint32_t chainImageIdx);
};

}
