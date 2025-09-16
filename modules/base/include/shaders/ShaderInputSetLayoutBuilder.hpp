#pragma once

#include "Gpu.hpp"

#include <vector>
#include <volk.h>
#include <assert.h>
#include <stdexcept>


class ShaderInputSetLayoutBuilder {
private:
	std::vector<VkDescriptorSetLayoutBinding> m_bindings;

public:
	ShaderInputSetLayoutBuilder() :
		m_bindings() {
	}

	ShaderInputSetLayoutBuilder(std::vector<VkDescriptorSetLayoutBinding> bindings) :
		m_bindings(bindings) {
	}

	ShaderInputSetLayoutBuilder& uniform_buffer(
            uint32_t binding, 
            VkShaderStageFlags shader_stages = VK_SHADER_STAGE_ALL
    ) {
		m_bindings.push_back({
			.binding = binding,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = shader_stages,
		});

		return *this;
	}

	ShaderInputSetLayoutBuilder& image(
            uint32_t binding, 
            VkShaderStageFlags shader_stages = VK_SHADER_STAGE_ALL
    ) {
		m_bindings.push_back({
			.binding = binding,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = shader_stages,
		});

		return *this;
	}


	ShaderInputSetLayoutBuilder& n_images(
            uint32_t binding, 
            uint32_t count,
            VkShaderStageFlags shader_stages = VK_SHADER_STAGE_ALL
    ) {
		m_bindings.push_back({
			.binding = binding,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = count,
			.stageFlags = shader_stages,
		});

		return *this;
	}

    ShaderInputSetLayoutBuilder& binding(
            uint32_t binding, 
            VkDescriptorType type, 
            uint32_t count,
            VkShaderStageFlags stages
    ) {
        m_bindings.push_back({
                .binding = binding,
                .descriptorType = type,
                .descriptorCount = count,
                .stageFlags = stages,
        });

        return *this;
    }

	VkDescriptorSetLayout build(const std::shared_ptr<const Gpu>& gpu) const {
		VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = (uint32_t)m_bindings.size(),
            .pBindings = m_bindings.data(),
        };

		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		if(vkCreateDescriptorSetLayout(gpu->dev(), &layoutInfo, nullptr, &layout)) {
			throw std::runtime_error("Failed to create descriptor set layout");
		}

		return layout;
	}
};
