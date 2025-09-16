#pragma once

#include <memory>
#include <stdexcept>
#include <volk.h>

#include "Gpu.hpp"

namespace lft {

class SamplerBuilder {
private:
    VkSamplerCreateInfo m_create_info;

public:
    SamplerBuilder() : m_create_info({}) {
        m_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    }

    inline SamplerBuilder& address_mode_u(VkSamplerAddressMode address_mode) {
        m_create_info.addressModeU = address_mode;
        return *this;
    }

    inline SamplerBuilder& address_mode_v(VkSamplerAddressMode address_mode) {
        m_create_info.addressModeV = address_mode;
        return *this;
    }

    inline SamplerBuilder& address_mode_w(VkSamplerAddressMode address_mode) {
        m_create_info.addressModeW = address_mode;
        return *this;
    }

    inline SamplerBuilder& address_mode(VkSamplerAddressMode address_mode) {
        address_mode_u(address_mode);
        address_mode_v(address_mode);
        address_mode_w(address_mode);
        return *this;
    }

    inline SamplerBuilder& min_filter(VkFilter filter) {
        m_create_info.minFilter = filter;
        return *this;
    }

    inline SamplerBuilder& mag_filter(VkFilter filter) {
        m_create_info.magFilter = filter;
        return *this;
    }

    inline SamplerBuilder& filter(VkFilter filter) {
        min_filter(filter);
        mag_filter(filter);
        return *this;
    }

    inline SamplerBuilder& mipmap_mode(VkSamplerMipmapMode mipmap_mode) {
        m_create_info.mipmapMode = mipmap_mode;
        return *this;
    }

    inline SamplerBuilder& border_color(VkBorderColor color) {
        m_create_info.borderColor = color;
        return *this;
    }

    inline SamplerBuilder& lod(float min_lod, float max_lod) {
        m_create_info.minLod = min_lod;
        m_create_info.maxLod = max_lod;
        return *this;
    }

    inline VkSampler build(std::shared_ptr<Gpu> gpu) {
        VkSampler sampler = VK_NULL_HANDLE;
        if(vkCreateSampler(gpu->dev(), &m_create_info, nullptr, &sampler)) {
            throw std::runtime_error("Failed to create sampler");
        }
        return sampler;
    }
};

}
