#include "shaders/ComputePipelineBuilder.hpp"

namespace lft {

ComputePipelineBuilder::ComputePipelineBuilder(const Shader& shader, VkPipelineLayout layout) :
    m_shader(shader),
    m_layout(layout) {

}

Pipeline ComputePipelineBuilder::build(std::shared_ptr<Gpu> gpu) {
    VkPipelineShaderStageCreateInfo computeShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = m_shader.module(),
        .pName = "main",
    };

    VkComputePipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = computeShaderStageInfo,
        .layout = m_layout,
    };

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (vkCreateComputePipelines(gpu->dev(),
                VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline!");
    }

    return Pipeline(m_layout, pipeline);
}

}
