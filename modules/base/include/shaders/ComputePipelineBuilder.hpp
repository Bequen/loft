#pragma once

#include "Shader.hpp"
#include "Pipeline.hpp"

#include <volk.h>

namespace lft {

class ComputePipelineBuilder {
private:
    const Shader* m_shader;
    VkPipelineLayout m_layout;

public:
    ComputePipelineBuilder(const Shader* shader, VkPipelineLayout layout);

    Pipeline build(const Gpu* gpu);
};

}
