//
// Created by martin on 10/24/23.
//

#include <stdexcept>
#include "shaders/SpirvShaderBuilder.hpp"

#include "io/file.hpp"

SpirvShaderBuilder::SpirvShaderBuilder(Gpu *pGpu) :
m_pGpu(pGpu) {

}

Shader SpirvShaderBuilder::from_file(std::string path) {
    size_t size = 0;
    uint32_t *pData = io::file::read_binary(path, &size);

    if(pData == nullptr) {
        throw std::runtime_error("Failed to read shader binary");
    }

    VkShaderModuleCreateInfo moduleInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = size,
            .pCode = pData,
    };

    VkShaderModule module = VK_NULL_HANDLE;
    if(vkCreateShaderModule(m_pGpu->dev(), &moduleInfo, nullptr, &module)) {
		throw std::runtime_error("Failed to create shader module");
	}

    auto shader = Shader(module);
    shader.set_name(m_pGpu, path);

    return shader;
}

