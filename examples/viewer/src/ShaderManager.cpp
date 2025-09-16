#include "ShaderManager.hpp"

ShaderManager::ShaderManager(std::shared_ptr<Gpu> gpu,
        std::string base_path) :
m_base_path(base_path),
m_shader_builder(gpu) {
}

void ShaderManager::load_shader(const std::string& name) {
    auto shader = m_shader_builder.from_file(m_base_path + name);
    m_shaders[name] = shader;
}

const Shader& ShaderManager::get(const std::string& name) {
    if(!m_shaders.contains(name)) {
        load_shader(name);
    }

    return m_shaders[name];
}
