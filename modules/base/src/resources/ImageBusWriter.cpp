#include "resources/ImageBusWriter.h"

#include <string.h>
#include <vulkan/vulkan_core.h>

int ImageBusWriter::create_staging_buffer(VkExtent2D extent, uint32_t formatSize,
										  uint32_t maxWrites) {
	BufferCreateInfo stagingBufferInfo = {
		.size = m_imageSize * maxWrites,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.isExclusive = true
	};

	MemoryAllocationInfo memoryAllocationInfo = {
		.usage = MEMORY_USAGE_AUTO_PREFER_HOST
	};

	m_pGpu->memory()->create_buffer(&stagingBufferInfo, &memoryAllocationInfo,
								&m_stagingBuffer);

    m_pGpu->memory()->map(m_stagingBuffer.allocation, &m_pMappedData);

	return 0;
}

int ImageBusWriter::create_staging_command_buffer() {
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = m_pGpu->transfer_command_pool(),
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	vkAllocateCommandBuffers(m_pGpu->dev(), &allocInfo,
							 &m_stagingCommandBuffer);

	return 0;
}

int ImageBusWriter::create_fence() {
    VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    vkCreateFence(m_pGpu->dev(), &fenceInfo, nullptr, &m_fence);

    return 0;
}

ImageBusWriter::ImageBusWriter(Gpu *pGpu, Image *pTarget,
							   VkExtent2D extent, uint32_t formatSize,
							   size_t maxWrites) :
m_pGpu(pGpu), m_pTarget(pTarget), m_writes(maxWrites), m_numWrites(0),
m_formatSize(formatSize), m_imageSize(extent.width * extent.height * formatSize) {
	create_staging_buffer(extent, formatSize, maxWrites);
	create_staging_command_buffer();
	create_fence();
}

void ImageBusWriter::write(VkBufferImageCopy write, void *pData) {
	write.bufferOffset = m_numWrites * m_imageSize;
	m_writes[m_numWrites] = write;

	memcpy((char*)m_pMappedData + m_numWrites * m_imageSize,
		   pData, 
		   write.imageExtent.width *
		   write.imageExtent.height *
		   m_formatSize);

	m_numWrites++;

	if(m_numWrites >= m_writes.size()) {
		flush();
	}
}

void ImageBusWriter::flush() {
	if(m_numWrites == 0) return;

	vkWaitForFences(m_pGpu->dev(), 1, &m_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_pGpu->dev(), 1, &m_fence);

	m_pGpu->memory()->flush(m_stagingBuffer.allocation, 0, 
							m_imageSize * m_numWrites);

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer(m_stagingCommandBuffer, &beginInfo);

	vkCmdCopyBufferToImage(
		m_stagingCommandBuffer,
		m_stagingBuffer.buf,
		m_pTarget->img,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		m_numWrites,
		m_writes.data()
	);

	vkEndCommandBuffer(m_stagingCommandBuffer);

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &m_stagingCommandBuffer
	};

	m_pGpu->enqueue_transfer(&submitInfo, m_fence);

	m_numWrites = 0;
}
