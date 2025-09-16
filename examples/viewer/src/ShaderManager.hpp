#pragma once

#include "shaders/Shader.hpp"
#include "shaders/SpirvShaderBuilder.hpp"

#include <string>
#include <unordered_map>

class ShaderManager {
private:
    std::string m_base_path;

    SpirvShaderBuilder m_shader_builder;
    std::unordered_map<std::string, Shader> m_shaders;

    void load_shader(const std::string& name);

public:
    ShaderManager(std::shared_ptr<Gpu> gpu, 
            std::string base_path = "");

    const Shader& get(const std::string& name);
};
