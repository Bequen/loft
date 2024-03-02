//
// Created by martin on 10/24/23.
//

#ifndef LOFT_SHADER_HPP
#define LOFT_SHADER_HPP

#include <vulkan/vulkan_core.h>

struct Shader {
private:
    VkShaderModule m_module;

public:
    Shader(VkShaderModule module) :
        m_module(module) {

    }

    const inline VkShaderModule module() const {
        return m_module;
    }
};

#endif //LOFT_SHADER_HPP
