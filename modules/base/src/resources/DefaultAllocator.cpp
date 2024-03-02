#include "resources/DefaultAllocator.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "Gpu.hpp"

DefaultAllocator::DefaultAllocator(Gpu *pGpu) {
    VmaAllocatorCreateInfo allocatorCreateInfo = {
            .physicalDevice = pGpu->gpu(),
            .device = pGpu->dev(),
            .instance = pGpu->instance().instance(),
            .vulkanApiVersion = VK_API_VERSION_1_2,
    };

    vmaCreateAllocator(&allocatorCreateInfo, &m_allocator);
}

VmaMemoryUsage get_vma_memory_usage(MemoryUsage memoryUsage) {
    switch(memoryUsage) {
        case MEMORY_USAGE_AUTO:
            return VMA_MEMORY_USAGE_AUTO;
        case MEMORY_USAGE_AUTO_PREFER_DEVICE:
            return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        case MEMORY_USAGE_AUTO_PREFER_HOST:
            return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        default:
            return VMA_MEMORY_USAGE_AUTO;
    }
}

int DefaultAllocator::create_buffer(BufferCreateInfo *pBufferInfo, MemoryAllocationInfo *pAllocInfo, Buffer *pOut) {
    VkBufferCreateInfo bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = pBufferInfo->size,
            .usage = pBufferInfo->usage,
            .sharingMode = pBufferInfo->isExclusive ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT
    };

    VmaAllocationCreateInfo allocInfo = {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			.usage = get_vma_memory_usage(pAllocInfo->usage),
			.requiredFlags = pAllocInfo->requiredFlags,
	};

    vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &pOut->buf, &pOut->allocation.allocation, nullptr);

    return 0;
}

int DefaultAllocator::create_image(ImageCreateInfo *pImageInfo, MemoryAllocationInfo *pAllocInfo, Image *pOut) {
    VkImageCreateInfo imageInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = pImageInfo->format,
            .extent = {
                    pImageInfo->extent.width,
                    pImageInfo->extent.height,
                    1,
            },
            .mipLevels = pImageInfo->mipLevels,
            .arrayLayers = pImageInfo->arrayLayers,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = pImageInfo->usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = get_vma_memory_usage(pAllocInfo->usage);
	allocInfo.requiredFlags = pAllocInfo->requiredFlags;

    vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &pOut->img, &pOut->allocation.allocation, nullptr);

    return 0;
}

