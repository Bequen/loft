//
// Created by martin on 10/24/23.
//

#include "../../include/shaders/GlslShaderBuilder.hpp"

Shader GlslShaderBuilder::from_file(std::string path) {
	Shader shader(VK_NULL_HANDLE, ShaderBinary({}));
	return shader;
}
