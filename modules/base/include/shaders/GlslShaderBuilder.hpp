#pragma once

#include "ShaderBuilder.hpp"

class GlslShaderBuilder : public ShaderBuilder {
public:
    Shader from_file(std::string path) override;
};
