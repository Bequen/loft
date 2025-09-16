#include "Recording.hpp"

namespace lft {


RecordingBindPoint::RecordingBindPoint(
        const Recording* recording,
        const Pipeline pipeline, 
        VkPipelineBindPoint bind_point
) : m_recording(recording),
m_pipeline(pipeline),
m_bind_point(bind_point) {
    vkCmdBindPipeline(m_recording->cmdbuf(), bind_point, pipeline.pipeline());
}

const RecordingBindPoint& RecordingBindPoint::bind_descriptor_set(
        uint32_t set,
        VkDescriptorSet descriptor_set
) const {
    vkCmdBindDescriptorSets(m_recording->cmdbuf(), 
            m_bind_point, 
            m_pipeline.pipeline_layout(), 
            set, 
            1, &descriptor_set, 
            0, nullptr);

    return *this;
}


const RecordingBindPoint& RecordingBindPoint::bind_descriptor_sets(
        uint32_t first_set,
        const std::vector<VkDescriptorSet>& descriptor_sets
) const {
    vkCmdBindDescriptorSets(m_recording->cmdbuf(), 
            m_bind_point, 
            m_pipeline.pipeline_layout(), 
            first_set,
            descriptor_sets.size(), descriptor_sets.data(), 
            0, nullptr);

    return *this;
}


const RecordingBindPoint& RecordingBindPoint::push_constants(
        VkShaderStageFlags shader_stages, 
        uint32_t offset, uint32_t size, const void* data) const {
    vkCmdPushConstants(m_recording->cmdbuf(), m_pipeline.pipeline_layout(), shader_stages,
            offset, size, data);
    return *this;
}

}

