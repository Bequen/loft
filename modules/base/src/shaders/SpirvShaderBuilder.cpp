//
// Created by martin on 10/24/23.
//

#include <stdexcept>
#include <iostream>
#include "shaders/SpirvShaderBuilder.hpp"

#include "io/file.hpp"

SpirvShaderBuilder::SpirvShaderBuilder(Gpu *pGpu) :
m_pGpu(pGpu) {

}

Shader SpirvShaderBuilder::from_file(std::string path) {
    std::cout << "Loading shader: " << path << std::endl;
    auto shaderBinary = io::file::read_binary(path);

    VkShaderModuleCreateInfo moduleInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = shaderBinary.code_size(),
            .pCode = shaderBinary.data().data(),
    };

    VkShaderModule module = VK_NULL_HANDLE;
    if(vkCreateShaderModule(m_pGpu->dev(), &moduleInfo, nullptr, &module)) {
		throw std::runtime_error("Failed to create shader module");
	}

    auto shader = Shader(module);
    shader.set_name(m_pGpu, path);

    return shader;
}

