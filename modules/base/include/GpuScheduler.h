//
// Created by martin on 11/12/23.
//

#ifndef LOFT_GPUSCHEDULER_H
#define LOFT_GPUSCHEDULER_H

#include <volk.h>

class GpuScheduler {
public:
    virtual void enqueue_transfer(VkSubmitInfo submitInfo) = 0;
    virtual void enqueue_graphics(VkSubmitInfo submitInfo) = 0;
    virtual void enqueue_present(VkPresentInfoKHR presentInfo) = 0;
};

#endif //LOFT_GPUSCHEDULER_H
