#include "resources/BufferBusWriter.h"
#include "../../../../examples/viewer/src/mesh/Vertex.hpp"

#include <string.h>
#include <volk.h>
#include <algorithm>


int
BufferBusWriter::create_staging_buffer(size_t size) {
	BufferCreateInfo stagingBufferInfo = {
		.size = size,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.isExclusive = true
	};

	MemoryAllocationInfo memoryAllocationInfo = {
		.usage = MEMORY_USAGE_AUTO_PREFER_HOST,
        .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

    m_gpu->memory()->create_buffer(&stagingBufferInfo, &memoryAllocationInfo,
								&m_stagingBuffer);

	return 0;
}



int
BufferBusWriter::create_staging_command_buffer() {
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = m_gpu->transfer_command_pool(),
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	vkAllocateCommandBuffers(m_gpu->dev(), &allocInfo, &m_stagingCommandBuffer);

	return 0;
}



BufferBusWriter::BufferBusWriter(const std::shared_ptr<const Gpu>& gpu, size_t size) :
m_gpu(gpu), m_unflushedSize(0), m_busSize(size), m_numWrites(0) {
	create_staging_buffer(size);

	create_staging_command_buffer();

    m_gpu->memory()->map(m_stagingBuffer.allocation, &m_pData);

    VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    vkCreateFence(m_gpu->dev(), &fenceInfo, nullptr, &m_fence);
}



void BufferBusWriter::write(Buffer* pTarget, void *pData, size_t offset, size_t size) {
    char* pUploadData = (char*)pData;

    while(size > 0) {
        size_t uploadSize = std::min(m_busSize - m_unflushedSize, size);

        memcpy((char*)m_pData + m_unflushedSize, pUploadData, uploadSize);
        pUploadData += uploadSize;

        VkBufferCopy write = {
                .srcOffset = m_unflushedSize,
                .dstOffset = offset,
                .size = uploadSize,
        };
        m_numWrites++;

        if(m_writes.size() <= m_numWrites) {
            m_writes.resize(m_numWrites * 2);
        }
        m_writes[m_numWrites - 1] = std::pair(pTarget, write);
        m_unflushedSize += uploadSize;

        offset += uploadSize;
        size -= uploadSize;

        if(m_busSize - m_unflushedSize == 0) {
            flush();
            // wait, because we are gonna overwrite the data
            wait();
        }
    }
}



void BufferBusWriter::flush() {
    if(m_numWrites == 0) {
        m_unflushedSize = 0;
        return;
    }

    m_gpu->memory()->flush(m_stagingBuffer.allocation, 0, m_unflushedSize);

	wait();
    vkResetFences(m_gpu->dev(), 1, &m_fence);

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	vkBeginCommandBuffer(m_stagingCommandBuffer, &beginInfo);

    for(uint32_t i = 0; i < m_numWrites; i++) {
        vkCmdCopyBuffer(m_stagingCommandBuffer, m_stagingBuffer.buf, m_writes[i].first->buf,
                        1, &m_writes[i].second);
    }

	vkEndCommandBuffer(m_stagingCommandBuffer);

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &m_stagingCommandBuffer,
	};

    m_gpu->enqueue_transfer(&submitInfo, m_fence);

    m_unflushedSize = 0;
	m_numWrites = 0;
}



BufferBusWriter::~BufferBusWriter() {
    flush();
    wait();

    m_gpu->memory()->destroy_buffer(&m_stagingBuffer);
}

void BufferBusWriter::wait() {
    vkWaitForFences(m_gpu->dev(), 1, &m_fence, VK_TRUE, UINT64_MAX);
    // vkDeviceWaitIdle(m_pGpu->dev());
}
