#include "shaders/PipelineBuilder.h"
#include "shaders/Shader.hpp"

PipelineBuilder::PipelineBuilder(const Gpu *pGpu, const VkViewport& viewport,
        VkPipelineLayout layout, VkRenderPass outputLayout,
        uint32_t numAttachments,
const Shader& vertexShader, const Shader& fragmentShader) :
m_pGpu(pGpu),
m_viewport(viewport),
m_scissor({
                  .offset = {0, 0},
                  .extent = {(uint32_t)viewport.width, (uint32_t)viewport.height}
          }),
m_layout(layout), m_renderpass(outputLayout),
m_inputAssemblyInfo({
                            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                            .primitiveRestartEnable = false
                    }),
m_rasterInfo({
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = false,
        .rasterizerDiscardEnable = false,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = false,
        .lineWidth = 1.0f
}),
m_depthStencilInfo({
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = true,
        .depthWriteEnable = true,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = false,
        .stencilTestEnable = false
}),
m_vertexInputInfo({
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
}),
m_blendingInfo(numAttachments)
{
    stages.resize(2);
    stages[0] = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShader.module(),
            .pName = "main"
    };

    stages[1] = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragmentShader.module(),
            .pName = "main",
    };

    for(uint32_t i = 0; i < num_attachments(); i++) {
        m_blendingInfo[i] = {
                .blendEnable = false,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                  VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT |
                                  VK_COLOR_COMPONENT_A_BIT
        };
    }
}

std::optional<Pipeline> PipelineBuilder::build() {
    m_vertexInputInfo.vertexBindingDescriptionCount = m_vertexBindings.size();
    m_vertexInputInfo.pVertexBindingDescriptions = (VkVertexInputBindingDescription*)m_vertexBindings.data();
    m_vertexInputInfo.vertexAttributeDescriptionCount = m_vertexAttributes.size();
    m_vertexInputInfo.pVertexAttributeDescriptions = (VkVertexInputAttributeDescription*)m_vertexAttributes.data();

    /* Multisampling disabled. Use TTA */
    VkPipelineMultisampleStateCreateInfo multisampling = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineViewportStateCreateInfo viewportState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &m_viewport,
            .scissorCount = 1,
            .pScissors = &m_scissor
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = (unsigned)m_blendingInfo.size(),
            .pAttachments = m_blendingInfo.data(),
            .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f}
    };

    VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = (uint32_t)stages.size(),
            .pStages = stages.data(),
            .pVertexInputState = &m_vertexInputInfo,
            .pInputAssemblyState = &m_inputAssemblyInfo,
            .pViewportState = &viewportState,
            .pRasterizationState = &m_rasterInfo,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &m_depthStencilInfo,
            .pColorBlendState = &colorBlending,
            .pDynamicState = nullptr,
            .layout = m_layout,
            .renderPass = m_renderpass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1,
    };

    VkPipeline pipeline;
    if(vkCreateGraphicsPipelines(m_pGpu->dev(), VK_NULL_HANDLE, 1,
                                 &pipelineInfo, nullptr, &pipeline)) {
        return {};
    }

    return Pipeline(m_layout, pipeline);
}