#include "resources/ImageBusWriter.h"

#include <string.h>
#include <volk/volk.h>
#include <cmath>

int ImageBusWriter::create_staging_buffer(size_t size) {
	BufferCreateInfo stagingBufferInfo = {
		.size = size,
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
	create_staging_buffer(extent.width * extent.height * formatSize);
	create_staging_command_buffer();
	create_fence();
}

void ImageBusWriter::write(VkBufferImageCopy write, void *pData, size_t size) {
	write.bufferOffset = m_numWrites * m_imageSize;
	m_writes[m_numWrites] = write;

	memcpy((char*)m_pMappedData + m_numWrites * m_imageSize, pData, size);

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

    VkImageMemoryBarrier barrierInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_pTarget->img,
    };

    std::vector<VkImageMemoryBarrier> barriers(m_writes.size(), barrierInfo);

    uint32_t i = 0;
    for(auto& write : m_writes) {
        barriers[i++].subresourceRange = {
                .aspectMask = write.imageSubresource.aspectMask,
                .baseMipLevel = write.imageSubresource.mipLevel,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = write.imageSubresource.baseArrayLayer,
                .layerCount = write.imageSubresource.layerCount,
        };
    }

    vkCmdPipelineBarrier(m_stagingCommandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         barriers.size(), barriers.data());

	vkCmdCopyBufferToImage(
		m_stagingCommandBuffer,
		m_stagingBuffer.buf,
		m_pTarget->img,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		m_numWrites,
		m_writes.data()
	);

    for(auto& barrier : barriers) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    vkCmdPipelineBarrier(m_stagingCommandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         barriers.size(), barriers.data());

	vkEndCommandBuffer(m_stagingCommandBuffer);

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &m_stagingCommandBuffer
	};

	m_pGpu->enqueue_transfer(&submitInfo, m_fence);

	m_numWrites = 0;
}
