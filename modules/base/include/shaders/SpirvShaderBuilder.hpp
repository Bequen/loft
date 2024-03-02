//
// Created by martin on 10/24/23.
//

#ifndef LOFT_SPIRVSHADERBUILDER_HPP
#define LOFT_SPIRVSHADERBUILDER_HPP

#include "ShaderBuilder.hpp"
#include "../Gpu.hpp"

class SpirvShaderBuilder : public ShaderBuilder {
private:
    Gpu *m_pGpu;

public:
    SpirvShaderBuilder(Gpu *pGpu);

    Shader from_file(std::string path) override;
};

#endif //LOFT_SPIRVSHADERBUILDER_HPP
