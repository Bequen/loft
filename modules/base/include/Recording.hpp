#pragma once

#include <optional>

#include "shaders/Pipeline.hpp"

namespace lft {

class RecordingBindPoint {
private:
    Pipeline m_pipeline;
    VkPipelineBindPoint m_bind_point;

public:
    RecordingBindPoint(
	    const Pipeline& pipeline, 
	    VkPipelineBindPoint bind_point
    );
};

class Recording {
private:
    std::optional<Pipeline> m_latest_graphics_pipeline;

public:
    /**
     * Binds pipeline. Will bind all the descriptor sets, etc. to it after.
     */
    Recording& bind_pipeline(const Pipeline& pipeline);

    Recording& bind_descriptor_set(VkPipelineBindPoint bind_point,
	    uint32_t set,
	    VkDescriptorSet descriptor_set,
	    uint32_t dynamic_offset);

    Recording& bind_descriptor_sets(
	    VkPipelineBindPoint bind_point,
	    uint32_t first_set,
	    const std::vector<VkDescriptorSet>& descriptor_sets,
	    const std::vector<uint32_t>& dynamic_offsets
    );

    Recording& draw(
	    uint32_t vertex_count, 
	    uint32_t instance_count,
	    uint32_t first_vertex,
	    uint32_t first_instance
    ) const;

    Recording& draw_offscreen() const;
};

}
