#pragma once

#include "Gpu.hpp"

#include <vector>
#include <vulkan/vulkan_core.h>
#include <assert.h>
#include <stdexcept>


class ShaderInputSetLayoutBuilder {
private:
	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	uint32_t m_numBindings;

public:
	ShaderInputSetLayoutBuilder(uint32_t numBindings) :
		m_bindings(numBindings), m_numBindings(0) {
	}

	ShaderInputSetLayoutBuilder(std::vector<VkDescriptorSetLayoutBinding> bindings) :
		m_bindings(bindings), m_numBindings(bindings.size()) {
	}

	ShaderInputSetLayoutBuilder& uniform_buffer(uint32_t binding) {
		assert(binding < m_bindings.size());

		m_bindings[m_numBindings++] = {
			.binding = binding,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL,
		};

		return *this;
	}

    ShaderInputSetLayoutBuilder& binding(uint32_t binding, VkDescriptorType type, uint32_t count, VkShaderStageFlags stages) {
        m_bindings[m_numBindings++] = {
                .binding = binding,
                .descriptorType = type,
                .descriptorCount = count,
                .stageFlags = stages,
        };

        return *this;
    }

	VkDescriptorSetLayout build(const Gpu *pGpu) {
		VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = (uint32_t)m_numBindings,
            .pBindings = m_bindings.data(),
        };

		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		if(vkCreateDescriptorSetLayout(pGpu->dev(), &layoutInfo, nullptr, &layout)) {
			throw std::runtime_error("Failed to create descriptor set layout");
		}

		return layout;
	}
};
