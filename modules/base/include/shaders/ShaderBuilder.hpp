//
// Created by martin on 10/24/23.
//

#ifndef LOFT_SHADERBUILDER_HPP
#define LOFT_SHADERBUILDER_HPP

#include <string>

#include "Shader.hpp"

class ShaderBuilder {
public:
    virtual Shader from_file(std::string path) = 0;
};

#endif //LOFT_SHADERBUILDER_HPP
