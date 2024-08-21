//
// Created by martin on 10/24/23.
//

#ifndef LOFT_SHADER_HPP
#define LOFT_SHADER_HPP

#include "Gpu.hpp"

#include <volk/volk.h>


struct Shader {
private:
    VkShaderModule m_module;

public:
    Shader(VkShaderModule module) :
        m_module(module) {

    }

    const inline Shader& set_name(const std::shared_ptr<const Gpu>& gpu, const std::string &name) {
#if LOFT_DEBUG
        VkDebugUtilsObjectNameInfoEXT nameInfo = {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                .objectType = VK_OBJECT_TYPE_SHADER_MODULE,
                .objectHandle = (uint64_t) m_module,
                .pObjectName = name.c_str(),
        };
        vkSetDebugUtilsObjectNameEXT(gpu->dev(), &nameInfo);
#endif

        return *this;
    }

    [[nodiscard]] inline VkShaderModule module() const {
        return m_module;
    }
};

#endif //LOFT_SHADER_HPP
