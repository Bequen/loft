#pragma once

#include <optional>
#include <algorithm>
#include <stdexcept>

#include "shaders/Pipeline.hpp"

namespace lft {

class Recording;

class RecordingBindPoint {
private:
    const Recording* m_recording;
    Pipeline m_pipeline;
    VkPipelineBindPoint m_bind_point;

public:
    RecordingBindPoint(
            const Recording* recording,
	    const Pipeline pipeline, 
	    VkPipelineBindPoint bind_point
    );

    const RecordingBindPoint& bind_descriptor_set(
            uint32_t set,
            VkDescriptorSet descriptor_set
    ) const;


    const RecordingBindPoint& bind_descriptor_sets(
	    uint32_t first_set,
	    const std::vector<VkDescriptorSet>& descriptor_sets
    ) const;

    const RecordingBindPoint& push_constants(
            VkShaderStageFlags shader_stages, 
            uint32_t offset, uint32_t size, const void* data
    ) const;
};

class Recording {
private:
    VkCommandBuffer m_cmdbuf;
    std::optional<RecordingBindPoint> m_latest_graphics_pipeline;

public:
    GET(m_cmdbuf, cmdbuf);

    Recording(VkCommandBuffer cmdbuf) : m_cmdbuf(cmdbuf) {}

    /**
     * Binds pipeline. Will bind all the descriptor sets, etc. to it after.
     */
    inline const RecordingBindPoint bind_graphics_pipeline(const Pipeline& pipeline) const {
        return RecordingBindPoint(this, pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS);
    }

    /**
     * Binds pipeline. Will bind all the descriptor sets, etc. to it after.
     */
    inline const RecordingBindPoint bind_compute_pipeline(const Pipeline& pipeline) const {
        return RecordingBindPoint(this, pipeline, VK_PIPELINE_BIND_POINT_COMPUTE);
    }

    inline const Recording& draw(
	    uint32_t vertex_count, 
	    uint32_t instance_count,
	    uint32_t first_vertex,
	    uint32_t first_instance
    ) const { 
        vkCmdDraw(m_cmdbuf, vertex_count, instance_count, first_vertex, first_instance);
        return *this;
    }

    inline const Recording& draw_indexed(
	    uint32_t index_count, 
	    uint32_t instance_count,
	    uint32_t first_index,
	    uint32_t vertex_offset,
        uint32_t first_instance
    ) const { 
        vkCmdDrawIndexed(m_cmdbuf, index_count, instance_count, first_index, vertex_offset, first_instance);
        return *this;
    }

    inline const Recording& bind_vertex_buffers(
            const std::vector<Buffer> buffers, 
            std::vector<std::size_t> offsets
    ) const {
        std::vector<VkBuffer> vertex_buffers(buffers.size());
        std::transform(buffers.begin(), buffers.end(), vertex_buffers.begin(),
                [](const Buffer& buffer) { return buffer.buf; });

        vkCmdBindVertexBuffers(m_cmdbuf, 0, vertex_buffers.size(), vertex_buffers.data(), offsets.data());
        return *this;
    }

    inline const Recording& bind_index_buffer(
            const Buffer index_buffer,
            std::size_t offset,
            VkIndexType index_type
    ) const {
        vkCmdBindIndexBuffer(m_cmdbuf, index_buffer.buf, offset, index_type);
        return *this;
    }

    inline const Recording& dispatch(
            uint32_t group_count_x, 
            uint32_t group_count_y, 
            uint32_t group_count_z
    ) const {
        vkCmdDispatch(m_cmdbuf, group_count_x, group_count_y, group_count_z);
        return *this;
    }

};

}
