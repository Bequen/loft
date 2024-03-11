#pragma once

#include "Gpu.hpp"

#include <vector>

class BufferBusWriter {
private:
    Gpu *m_pGpu;
    Buffer *m_pTarget;

    Buffer m_stagingBuffer;
    size_t m_busSize;
	VkCommandBuffer m_stagingCommandBuffer;

    void *m_pData;
    size_t m_unflushedSize;

	uint32_t m_numWrites;
	std::vector<VkBufferCopy> m_writes;

	VkFence m_fence;

	int create_staging_command_buffer();
	int create_staging_buffer(size_t size);

public:
    BufferBusWriter(Gpu *pGpu, Buffer *pBuffer, size_t size);
    ~BufferBusWriter();

	inline BufferBusWriter& set_buffer(Buffer *pBuffer) {
        flush();
        wait();

		m_pTarget = pBuffer;
		return *this;
	}

    void write(void *pData, size_t offset, size_t size);
    void flush();

    void wait();
};
