//
// Created by martin on 10/24/23.
//

#include <stdexcept>

#include "shaders/SpirvShaderBuilder.hpp"
#include "io/file.hpp"

SpirvShaderBuilder::SpirvShaderBuilder(const Gpu* gpu) :
        m_gpu(gpu) {

}

Shader SpirvShaderBuilder::from_binary(const std::vector<uint32_t>& code) const {
    VkShaderModuleCreateInfo moduleInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size(),
            .pCode = code.data(),
    };

    VkShaderModule module = VK_NULL_HANDLE;
    if(vkCreateShaderModule(m_gpu->dev(), &moduleInfo, nullptr, &module)) {
		throw std::runtime_error(std::format("Failed to create shader module from binary. Length was {}", code.size() * sizeof(uint32_t)));
	}

    return Shader(module);
}

Shader SpirvShaderBuilder::from_file(std::string path) {
    auto shaderBinary = io::file::read_binary(path);

    VkShaderModuleCreateInfo moduleInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = shaderBinary.code_size(),
            .pCode = shaderBinary.data().data(),
    };

    VkShaderModule module = VK_NULL_HANDLE;
    if(vkCreateShaderModule(m_gpu->dev(), &moduleInfo, nullptr, &module)) {
		throw std::runtime_error("Failed to create shader module");
	}

    auto shader = Shader(module);
    shader.set_name(m_gpu, path);

    return shader;
}
