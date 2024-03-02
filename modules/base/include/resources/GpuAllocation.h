#pragma once

#include "vk_mem_alloc.h"

/*
 * Gpu memory node.
 */
struct GpuAllocation {
    VmaAllocation allocation;
};
