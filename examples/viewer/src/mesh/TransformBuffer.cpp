#include "TransformBuffer.hpp"
#include "mesh/Transform.hpp"

namespace lft::scene {

void TransformBuffer::allocate_buffer(size_t num_transforms) {
    MemoryAllocationInfo memory_info = {
            .usage = MEMORY_USAGE_AUTO_PREFER_DEVICE,
            .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    };

    BufferCreateInfo transform_buffer_info = {
            .size = sizeof(Transform) * num_transforms,
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .isExclusive = true,
    };

    m_gpu->memory()->create_buffer(&transform_buffer_info, &memory_info,
                                  &m_transform_buffer);
    m_transform_buffer.set_debug_name(m_gpu, "transform_buffer");
}

TransformBuffer::TransformBuffer(const Gpu* gpu, size_t num_transforms) :
    m_gpu(gpu),
    m_transform_buffer(),
    m_writer(gpu, 100 * sizeof(Transform))
{
    allocate_buffer(num_transforms);
}

}
