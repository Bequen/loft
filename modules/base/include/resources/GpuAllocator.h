#pragma once

#include "Image.hpp"
#include "Buffer.hpp"
#include "GpuAllocation.h"

struct BufferCreateInfo {
    size_t size;
    VkBufferUsageFlags usage;

    bool isExclusive;
};

struct ImageCreateInfo {
    VkExtent2D extent;
    VkFormat format;

    VkImageUsageFlags usage;
    VkImageAspectFlags aspectMask;

    uint32_t arrayLayers;
    uint32_t mipLevels;
};

enum MemoryUsage {
    MEMORY_USAGE_AUTO = 0,
    MEMORY_USAGE_AUTO_PREFER_DEVICE = 1,
    MEMORY_USAGE_AUTO_PREFER_HOST = 2
};

struct MemoryAllocationInfo {
    MemoryUsage usage;
	VkMemoryPropertyFlags requiredFlags;
};

class Gpu;
/**
 * Allocating of memory
 */
class GpuAllocator {
protected:

public:
    virtual int create_buffer(BufferCreateInfo *pBufferInfo, 
							  MemoryAllocationInfo *pAllocInfo, Buffer *pOut) = 0;
    virtual int create_image(ImageCreateInfo *pImageInfo,
							 MemoryAllocationInfo *pAllocInfo, Image *pOut) = 0;

    virtual void map(GpuAllocation& allocation, void **pData) = 0;
    virtual void unmap(GpuAllocation& allocation) = 0;

    virtual void destroy_buffer(Buffer *pBuffer) = 0;
    virtual void destroy_image(Image *pImage) = 0;

	virtual VkResult flush(GpuAllocation& allocation,
						   size_t offset, size_t size) = 0;
};
