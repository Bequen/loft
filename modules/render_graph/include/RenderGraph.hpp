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
	const Gpu* m_gpu;
    
    std::string m_output_name;

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
            VkSemaphore wait_semaphore,
			VkFence fence,
			uint32_t output_idx
	);


    bool is_batch_writing_to_final_image(const Batch& buffer) const;


public:
	const RenderGraphBuffer& buffer(uint32_t idx) const {
	    return *m_buffers[idx];
	}

	RenderGraph& invalidate(const std::string& name);

	RenderGraph(const Gpu* gpu,
                const std::string& output_name,
	            const std::vector<RenderGraphBuffer*>& buffers,
				AdjacencyMatrix* dependencies
    );

    /**
     * Runs the render graph. Outputs to final image.
     * @param chainImageIdx index of image in the final image chain
     * @param final_image_fence fence to wait on for final image. The render graph will attempt to run as much tasks before waiting as possible.
     */
    void run(uint32_t chainImageIdx, VkSemaphore semaphore, VkFence final_image_fence);
};

}
