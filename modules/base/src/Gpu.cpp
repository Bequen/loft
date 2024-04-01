#include "Gpu.hpp"

#include "resources/DefaultAllocator.h"
#include "result.hpp"

#include <vector>
#include <iostream>

static const char *DEVICE_EXTENSIONS[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static const int NUM_DEVICE_EXTENSIONS = sizeof(DEVICE_EXTENSIONS) / sizeof(*DEVICE_EXTENSIONS);


static const char* LAYERS[] = {
        "VK_LAYER_KHRONOS_validation"
};

static const int NUM_LAYERS = sizeof(LAYERS) / sizeof(*LAYERS);


int32_t get_graphics_score(VkQueueFamilyProperties props) {
    bool isSupported = props.queueFlags | VK_QUEUE_GRAPHICS_BIT;
    return isSupported;
}

int32_t get_transfer_score(VkQueueFamilyProperties props) {
    bool isSupported = props.queueFlags | VK_QUEUE_TRANSFER_BIT;
    bool isDedicated = props.queueFlags & ~(VK_QUEUE_GRAPHICS_BIT);

    return (isDedicated << 1) * isSupported;
}

int32_t get_present_score(VkQueueFamilyProperties props, bool isPresentSupported) {
    return isPresentSupported;
}

std::vector<int32_t> Gpu::get_queues(VkSurfaceKHR surface) {
    uint32_t numQueues = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &numQueues, 0x0);
    auto properties = std::vector<VkQueueFamilyProperties>(numQueues);
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &numQueues,
                                             properties.data());

    int32_t graphicsQueue = -1;

    int32_t transferQueue = -1;
    int32_t transferScore = 0;

    int32_t presentQueue = -1;
    int32_t presentScore = 0;

    uint32_t i = 0;
    for(auto& prop : properties) {
        VkBool32 isPresentSupported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu, i, surface, &isPresentSupported);

        bool isTaken = false;
        int32_t iterGraphicsScore = get_graphics_score(prop) * (graphicsQueue == -1);
        int32_t iterTransferScore = get_transfer_score(prop);
        int32_t iterPresentScore = get_present_score(prop, isPresentSupported);

        if(iterGraphicsScore > 0) {
            graphicsQueue = i;
            isTaken = true;
        }

        if((iterTransferScore | (!isTaken << 2)) > transferScore) {
            transferQueue = i;
            transferScore = (iterTransferScore & (!isTaken << 2));
            isTaken = true;
        }

        if((iterPresentScore | (!isTaken << 2)) > presentScore) {
            presentQueue = i;
            presentScore = (iterPresentScore & (!isTaken << 2));
            isTaken = true;
        }
    }

    return { graphicsQueue, transferQueue, presentQueue };
}

Result Gpu::create_logical_device(VkSurfaceKHR supportedSurface) {
    auto queueFamilies = get_queues(supportedSurface);

    std::vector<VkDeviceQueueCreateInfo> queueInfos(queueFamilies.size());
    float priority = 1.0f;

    int32_t x = 0;
    for(int32_t i = 0; i < queueFamilies.size(); i++) {
        bool isDuplicate = false;
        for(int32_t y = i - 1; y >= 0; y--) {
            if(queueFamilies[i] == queueFamilies[y]) {
                isDuplicate = true;
                break;
            }
        }

        if(isDuplicate) continue;

        queueInfos[x] = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = (uint32_t)queueFamilies[x],
                .queueCount = 1,
                .pQueuePriorities = &priority
        };
        x++;
    }

    VkPhysicalDeviceSynchronization2Features syncFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
            .synchronization2 = true
    };

	VkPhysicalDeviceFeatures gpuFeatures = {

	};

	VkPhysicalDeviceCoherentMemoryFeaturesAMD coherentMemoryFeatures {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD,
        .pNext = &syncFeatures,
		.deviceCoherentMemory = true
	};

	VkDeviceCreateInfo deviceInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &syncFeatures, //&coherentMemoryFeatures,
		.queueCreateInfoCount = (uint32_t)x,
		.pQueueCreateInfos = queueInfos.data(),
		.enabledLayerCount = NUM_LAYERS,
		.ppEnabledLayerNames = LAYERS,
		.enabledExtensionCount = NUM_DEVICE_EXTENSIONS,
		.ppEnabledExtensionNames = DEVICE_EXTENSIONS,
		.pEnabledFeatures = &gpuFeatures
	};

	if(vkCreateDevice(m_gpu, &deviceInfo, NULL, &m_dev)) {
		return RESULT_GPU_DEVICE_CREATION_FAILED;
	}

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = (uint32_t)queueFamilies[0],
    };

    if(vkCreateCommandPool(m_dev, &poolInfo, nullptr, &m_graphicsCommandPool)) {
        return RESULT_GPU_COMMAND_POOL_CREATION_FAILED;
    }

    vkGetDeviceQueue(m_dev, queueFamilies[0], 0, &m_graphicsQueue);
    if(queueFamilies[0] == queueFamilies[1]) {
        m_transferCommandPool = m_graphicsCommandPool;
        m_transferQueue = m_graphicsQueue;
    } else {
        poolInfo.queueFamilyIndex = (uint32_t)queueFamilies[1];
        if(vkCreateCommandPool(m_dev, &poolInfo, nullptr, &m_graphicsCommandPool)) {
            return RESULT_GPU_COMMAND_POOL_CREATION_FAILED;
        }
        vkGetDeviceQueue(m_dev, queueFamilies[1], 0, &m_transferQueue);
    }

    if(queueFamilies[0] == queueFamilies[2]) {
        m_presentQueue = m_graphicsQueue;
        m_presentCommandPool = m_graphicsCommandPool;
    } else if(queueFamilies[1] == queueFamilies[2]) {
        m_presentQueue = m_transferQueue;
        m_presentCommandPool = m_transferCommandPool;
    } else {
        poolInfo.queueFamilyIndex = (uint32_t)queueFamilies[2];
        if(vkCreateCommandPool(m_dev, &poolInfo, nullptr, &m_graphicsCommandPool)) {
            return RESULT_GPU_COMMAND_POOL_CREATION_FAILED;
        }
        vkGetDeviceQueue(m_dev, queueFamilies[2], 0, &m_presentQueue);
    }

    m_graphicsQueueIdx = queueFamilies[0];
    m_transferQueueIdx = queueFamilies[1];
    m_presentQueueIdx = queueFamilies[2];

	return RESULT_OK;
}

Result Gpu::choose_gpu(VkPhysicalDevice *pOut) {

	uint32_t numDevices = 0;
	vkEnumeratePhysicalDevices(m_instance.instance(), &numDevices, nullptr);
    if(numDevices == 0) {
        return RESULT_NO_AVAILABLE_GPU;
    }

	auto devices = std::vector<VkPhysicalDevice>(numDevices);
	vkEnumeratePhysicalDevices(m_instance.instance(), &numDevices, devices.data());

    printf("num devices: %ld\n", devices.size());

	VkPhysicalDevice chosen = devices[0];
	*pOut = chosen;

	return RESULT_OK;
}

Result Gpu::create_descriptor_pool() {
    VkDescriptorPoolSize poolSizes[] =
    {
        {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1000,
        }, {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1000
        }
    };

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1000,
        .poolSizeCount = sizeof(poolSizes) / sizeof(*poolSizes),
        .pPoolSizes = poolSizes,
    };

    if (vkCreateDescriptorPool(dev(), &poolInfo, nullptr, &m_descriptorPool)) {
        return RESULT_GPU_DESCRIPTOR_POOL_CREATION_FAILED;
    }

    return RESULT_OK;
}


Gpu::Gpu(Instance instance, VkSurfaceKHR supportedSurface) :
    m_instance(instance), m_pAllocator(nullptr) {

	if(choose_gpu(&m_gpu)) {
		throw std::runtime_error("Failed to choose gpu");
	}

    if(create_logical_device(supportedSurface)) {
        throw std::runtime_error("Failed to create logical device");
    }

    if(create_descriptor_pool()) {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    m_pAllocator = new DefaultAllocator(this);
}

void Gpu::enqueue_present(VkPresentInfoKHR *pPresentInfo) const {
    vkQueuePresentKHR(m_presentQueue, pPresentInfo);
}

void Gpu::enqueue_graphics(VkSubmitInfo2 *pSubmitInfo, VkFence fence) const {
    if(vkQueueSubmit2(m_graphicsQueue, 1, pSubmitInfo, fence)) {
        throw std::runtime_error("Failed to submit to graphics queue");
    }
}

void Gpu::enqueue_transfer(VkSubmitInfo *pSubmitInfo, VkFence fence) const {
    vkQueueSubmit(m_transferQueue, 1, pSubmitInfo, fence);
}
