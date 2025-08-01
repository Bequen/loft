#pragma once

#include <volk.h>

/**
 * Surface Interface
 * providing function to retrieve surface.
 * Used by GPU.
 */
class Surface {
public:
    [[nodiscard("Only one surface should be created")]]
    virtual VkSurfaceKHR create_surface(VkInstance instance) = 0;
};