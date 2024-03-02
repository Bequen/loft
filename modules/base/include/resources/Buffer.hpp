#pragma once

#include <vulkan/vulkan_core.h>
#include "GpuAllocation.h"

/**
 * Buffer
 * Device memory suballocation with generic data inside.
 */
struct Buffer {
	VkBuffer buf;
    GpuAllocation allocation;

    Buffer() : Buffer(VK_NULL_HANDLE, {}) {

    }


    Buffer(VkBuffer buffer, GpuAllocation allocation) :
    buf(buffer), allocation(allocation) {

    }

    Buffer(Buffer&& a)  noexcept :
    buf(a.buf), allocation(a.allocation) {

    }

    /* disable copy */
    // void operator=(Buffer&) = delete;
    // Buffer(Buffer& a) = delete;
};
