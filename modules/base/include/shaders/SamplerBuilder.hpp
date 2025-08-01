#pragma once

#include <volk.h>

#include <memory>
#include <vulkan/vulkan_core.h>

#include "Gpu.hpp"

namespace lft {
    class SamplerBuilder {
	VkSamplerCreateInfo m_sampler_info = {};

	public:
	    SamplerBuilder() {
		m_sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	    }

	    inline SamplerBuilder& mag_filter(VkFilter filter) {
		m_sampler_info.magFilter = filter;
		return *this;
	    }

	    inline SamplerBuilder& min_filter(VkFilter filter) {
		m_sampler_info.minFilter = filter;
		return *this;
	    }

	    inline SamplerBuilder& filter(VkFilter filter) {
		mag_filter(filter);
		min_filter(filter);
		return *this;
	    }

	    inline SamplerBuilder& address_mode_u(VkSamplerAddressMode address_mode) {
		m_sampler_info.addressModeU = address_mode;
		return *this;
	    }


	    inline SamplerBuilder& address_mode_v(VkSamplerAddressMode address_mode) {
		m_sampler_info.addressModeV = address_mode;
		return *this;
	    }

	    inline SamplerBuilder& address_mode_w(VkSamplerAddressMode address_mode) {
		m_sampler_info.addressModeW = address_mode;
		return *this;
	    }

	    inline SamplerBuilder& address_mode(VkSamplerAddressMode address_mode) {
		address_mode_u(address_mode);
		address_mode_v(address_mode);
		address_mode_w(address_mode);
		return *this;
	    }

	    inline SamplerBuilder& border_color(VkBorderColor border_color) {
		m_sampler_info.borderColor = border_color;
		return *this;
	    }

	    VkSampler build(std::shared_ptr<Gpu> gpu) {
		VkSampler sampler = VK_NULL_HANDLE;
		if(vkCreateSampler(gpu->dev(), &m_sampler_info, nullptr, &sampler)) {
		    throw std::runtime_error("failed to create sampler");
		}

		return sampler;
	    }
    };
}
