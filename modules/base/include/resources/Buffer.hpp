#pragma once

#include <string>
#include <volk/volk.h>
#include <memory>
#include "GpuAllocation.h"

class Gpu;

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

    Buffer(Buffer& a)  noexcept :
    buf(a.buf), allocation(a.allocation) {

    }


    void set_debug_name(const std::shared_ptr<const Gpu>& gpu, const std::string& name) const;

    /* disable copy */
};
