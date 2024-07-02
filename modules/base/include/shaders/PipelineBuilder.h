#include "Gpu.hpp"
#include "Shader.hpp"
#include "VertexBinding.h"
#include "VertexAttribute.h"
#include "BlendingInfo.h"
#include "Pipeline.hpp"
#include <volk/volk.h>
#include <stdexcept>

class PipelineLayoutBuilder {
private:
	std::vector<VkDescriptorSetLayout> m_layouts;
	std::vector<VkPushConstantRange> m_pushConstantRanges;

	uint32_t m_numLayouts;

public:
	PipelineLayoutBuilder() :
	m_layouts(4), m_pushConstantRanges(), m_numLayouts(0) {
	}

	PipelineLayoutBuilder& input_set(uint32_t idx, VkDescriptorSetLayout setLayout) {
		if(idx >= m_layouts.size()) {
			throw std::runtime_error("As of now, only 4 descriptor set layouts are supported");
		}

		m_layouts[idx] = setLayout;
		m_numLayouts = std::max(m_numLayouts, idx + 1);
		return *this;
	}

    PipelineLayoutBuilder& push_constant_range(uint32_t offset, uint32_t size, VkShaderStageFlags stages) {
        m_pushConstantRanges.push_back({
            .stageFlags = stages,
            .offset = offset,
            .size = size,
        });
        return *this;
    }

	VkPipelineLayout build(const Gpu *pGpu) {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = m_numLayouts,
                .pSetLayouts = m_layouts.data(),
                .pushConstantRangeCount = (uint32_t)m_pushConstantRanges.size(),
                .pPushConstantRanges = m_pushConstantRanges.data(),
        };

        VkPipelineLayout inputLayout = VK_NULL_HANDLE;
        if (vkCreatePipelineLayout(pGpu->dev(), &pipelineLayoutInfo, nullptr, &inputLayout)) {
            throw std::runtime_error("Failed to build pipeline layout");
        }

        return inputLayout;
    }
};

class PipelineBuilder {
private:
    const Gpu *m_pGpu;

    VkPipelineLayout m_layout;
    VkRenderPass m_renderpass;

    VkViewport m_viewport;
    VkRect2D m_scissor;

    VkPipelineRasterizationStateCreateInfo m_rasterInfo;
    VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyInfo;
    VkPipelineDepthStencilStateCreateInfo m_depthStencilInfo;
    VkPipelineVertexInputStateCreateInfo m_vertexInputInfo;

    std::vector<VkPipelineColorBlendAttachmentState> m_blendingInfo;
    std::vector<VkPipelineShaderStageCreateInfo> stages;

    std::vector<VertexBinding> m_vertexBindings;
    std::vector<VertexAttribute> m_vertexAttributes;

public:
    inline size_t num_attachments() { return m_blendingInfo.size(); }

    PipelineBuilder(const Gpu *pGpu, const VkViewport& viewport,
                    VkPipelineLayout layout, VkRenderPass outputLayout,
                    uint32_t numAttachments,
                    const Shader& vertexShader, const Shader& fragmentShader);

    inline PipelineBuilder& set_vertex_input_info(std::vector<VertexBinding> bindings,
                                                  std::vector<VertexAttribute> attributes) {
        m_vertexBindings = bindings;
        m_vertexAttributes = attributes;
        return *this;
    }

    /* Rasterization */
    inline PipelineBuilder& polygon_mode(VkPolygonMode mode) {
        this->m_rasterInfo.polygonMode = mode;
        return *this;
    }

    inline PipelineBuilder& cull_mode(VkCullModeFlags flags) {
        this->m_rasterInfo.cullMode = flags;
        return *this;
    }

    inline PipelineBuilder& topology(VkPrimitiveTopology topology) {
        this->m_inputAssemblyInfo.topology = topology;
        return *this;
    }

    inline PipelineBuilder& set_depth_bias(float depthBiasConstantFactor, float depthBiasClamp,
                                           float depthBiasSlopeFactor) {
        m_rasterInfo.depthBiasEnable = true,
		m_rasterInfo.depthBiasConstantFactor = depthBiasConstantFactor;
        m_rasterInfo.depthBiasClamp = depthBiasClamp;
        m_rasterInfo.depthBiasSlopeFactor = depthBiasSlopeFactor;
        return *this;
    }

    inline PipelineBuilder& unset_blending(uint32_t attachmentIdx) {
        m_blendingInfo[attachmentIdx].blendEnable = false;
        return *this;
    }

    inline PipelineBuilder& set_blending(uint32_t attachmentIdx, BlendingInfo blendingInfo) {
        m_blendingInfo[attachmentIdx] = {
                .blendEnable = true,
                .srcColorBlendFactor = blendingInfo.srcColorBlendFactor,
                .dstColorBlendFactor = blendingInfo.dstColorBlendFactor,
                .colorBlendOp = blendingInfo.colorBlendOp,
                .srcAlphaBlendFactor = blendingInfo.srcAlphaBlendFactor,
                .dstAlphaBlendFactor = blendingInfo.dstAlphaBlendFactor,
                .alphaBlendOp = blendingInfo.alphaBlendOp,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                  VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT |
                                  VK_COLOR_COMPONENT_A_BIT
        };
        return *this;
    }

    std::optional<Pipeline> build();
};
