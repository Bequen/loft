#pragma once

#include "resources/Buffer.hpp"
#include "Vertex.hpp"
#include "resources/BufferBusWriter.h"

class MeshBuffer {
    Buffer m_vertexBuffer;
    Buffer m_indexBuffer;

public:
    MeshBuffer(const Gpu *pGpu,
               BufferBusWriter *pWriter,
               const std::vector<Vertex>& vertices,
               const std::vector<Index>& indices) {
        MemoryAllocationInfo memoryAllocationInfo = {
                .usage = MEMORY_USAGE_AUTO_PREFER_DEVICE,
        };

        BufferCreateInfo vertexBufferInfo = {
                .size = vertices.size() * sizeof(Vertex),
                .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .isExclusive = true,
        };
        pGpu->memory()->create_buffer(&vertexBufferInfo, &memoryAllocationInfo,
                                      &m_vertexBuffer);

        pWriter->set_buffer(&m_vertexBuffer);
        pWriter->write((void*)vertices.data(), 0, vertexBufferInfo.size);


        BufferCreateInfo indexBufferInfo = {
                .size = indices.size() * sizeof(Index),
                .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .isExclusive = true,
        };
        pGpu->memory()->create_buffer(&indexBufferInfo, &memoryAllocationInfo,
                                      &m_indexBuffer);

        pWriter->set_buffer(&m_indexBuffer);
        pWriter->write((void*)indices.data(), 0, indexBufferInfo.size);

        pWriter->wait();
    }
};