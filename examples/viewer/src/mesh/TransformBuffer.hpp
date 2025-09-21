#pragma once

#include "resources/BufferBusWriter.h"
#include "resources/Buffer.hpp"

namespace lft::scene {

class TransformBuffer {
    const Gpu* m_gpu;

    Buffer m_transform_buffer;

    /* always ready buffer writer */
    BufferBusWriter m_writer;

    void allocate_buffer(size_t num_transforms);

public:
    TransformBuffer(const Gpu* gpu, size_t num_transforms);
};

}
