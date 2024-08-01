#pragma once

#include "Gpu.hpp"

#include <vector>

/**
 * Writes data to a buffer by using a staging buffer
 */
class BufferBusWriter {
private:
    std::shared_ptr<const Gpu> m_gpu;

    Buffer m_stagingBuffer;
    size_t m_busSize;
	VkCommandBuffer m_stagingCommandBuffer;

    void *m_pData;
    size_t m_unflushedSize;

	uint32_t m_numWrites;
	std::vector<std::pair<Buffer*, VkBufferCopy>> m_writes;

	VkFence m_fence;

	int create_staging_command_buffer();
	int create_staging_buffer(size_t size);

public:
    BufferBusWriter(const std::shared_ptr<const Gpu>& gpu, size_t size);
    ~BufferBusWriter();

    void write(Buffer* pTarget, void *pData, size_t offset, size_t size);
    void flush();

    void wait();
};
