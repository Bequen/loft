#pragma once

#include "Gpu.hpp"

class ImageBusWriter {
private:
    std::shared_ptr<const Gpu> m_gpu;
	
	/**
	 * Image to which we are writing
	 */
	Image *m_pTarget;
	uint32_t m_formatSize;
	uint32_t m_imageSize;
	

	/**
	 * Intermediate staging buffer to pass data from heap 3 to heap 0
	 */
	Buffer m_stagingBuffer;
	size_t m_stagingBufferSize;
	VkCommandBuffer m_stagingCommandBuffer;

	/**
	 * Pointing to where the staging buffer is mapped
	 */
	void *m_pMappedData;


	std::vector<VkBufferImageCopy> m_writes;
	uint32_t m_numWrites;

	VkFence m_fence;

	int create_staging_buffer(size_t size);

	int create_staging_command_buffer();

	int create_fence();

public:
	ImageBusWriter(const std::shared_ptr<const Gpu>& gpu, Image *pImage,
				   VkExtent2D extent, uint32_t formatSize,
				   size_t maxWrites);

	void write(VkBufferImageCopy region, void *pData, size_t size);

    void set_target(Image *pTarget) {
        flush();
        m_pTarget = pTarget;
    }

	void flush();
};
