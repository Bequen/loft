#pragma once

#include "resources/GpuAllocator.h"

#include "vk_mem_alloc.h"

class DefaultAllocator : public GpuAllocator {
private:
    VmaAllocator m_allocator;

public:
    explicit DefaultAllocator(Gpu *pGpu);

    int create_image(ImageCreateInfo *pImageInfo,
					 MemoryAllocationInfo *pAllocInfo,
					 Image *pOut) override;

    int create_buffer(BufferCreateInfo *pBufferInfo, 
					  MemoryAllocationInfo *pAllocInfo,
					  Buffer *pOut) override;

    void destroy_buffer(Buffer *pBuffer) override {

    }
    void destroy_image(Image *pImage) override {

    }

    inline void map(GpuAllocation& allocation, void **pData) override {
        vmaMapMemory(m_allocator, allocation.allocation, pData);
    }

    inline void unmap(GpuAllocation& allocation) override {
        vmaUnmapMemory(m_allocator, allocation.allocation);
    }

	inline VkResult flush(GpuAllocation& allocation, size_t offset, size_t size) override {
		vmaFlushAllocation(m_allocator, allocation.allocation, offset, size);
        return vmaInvalidateAllocation(m_allocator, allocation.allocation, offset, size);
	}
};
