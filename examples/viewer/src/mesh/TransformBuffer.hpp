#pragma once

#include "resources/BufferBusWriter.h"
#include "resources/Buffer.hpp"

namespace lft::scene {

class TransformBuffer {
    std::shared_ptr<Gpu> m_gpu;

    Buffer m_transform_buffer;

    /* always ready buffer writer */
    BufferBusWriter m_writer;

    void allocate_buffer(size_t num_transforms);

public:
    TransformBuffer(std::shared_ptr<Gpu> gpu, size_t num_transforms);
};

}
