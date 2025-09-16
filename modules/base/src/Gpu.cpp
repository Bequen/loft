#include "Gpu.hpp"

#include "resources/DefaultAllocator.h"
#include "result.hpp"

#include <vector>
#include <iostream>
#include <memory>
#include <cstring>
#include <vulkan/vulkan_core.h>

static const char *DEVICE_EXTENSIONS[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME
};

static const int NUM_DEVICE_EXTENSIONS = sizeof(DEVICE_EXTENSIONS) / sizeof(*DEVICE_EXTENSIONS);

#if VK_LAYERS_ENABLE
static const char* LAYERS[] = {
        "VK_LAYER_KHRONOS_validation"
};
static const int NUM_LAYERS = sizeof(LAYERS) / sizeof(*LAYERS);

#else
#define LAYERS nullptr
static const int NUM_LAYERS = 0;

#endif



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

std::vector<int32_t> Gpu::get_queues(std::optional<VkSurfaceKHR> surface) {
    uint32_t numQueues = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &numQueues, 0x0);
    auto properties = std::vector<VkQueueFamilyProperties>(numQueues);
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &numQueues,
                                             properties.data());

    if(numQueues == 0) {
        throw std::runtime_error("Failed to find any queue family");
    }

    int32_t graphicsQueue = -1;

    int32_t transferQueue = -1;
    int32_t transferScore = 0;

    int32_t presentQueue = -1;
    int32_t presentScore = 0;

    uint32_t i = 0;
    for(auto& prop : properties) {
        VkBool32 isPresentSupported = false;
        if(surface.has_value()) {
            vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu, i, surface.value(), &isPresentSupported);
        } else {
            isPresentSupported = true;
        }

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

Result Gpu::create_logical_device(std::optional<VkSurfaceKHR> supportedSurface) {
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

	VkPhysicalDeviceFeatures gpuFeatures = { };
    
    vkGetPhysicalDeviceFeatures(m_gpu, &gpuFeatures);

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
#if LOFT_DEBUG
		.enabledLayerCount = NUM_LAYERS,
		.ppEnabledLayerNames = LAYERS,
#endif
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
        if(vkCreateCommandPool(m_dev, &poolInfo, nullptr, &m_transferCommandPool)) {
            return RESULT_GPU_COMMAND_POOL_CREATION_FAILED;
        }
        m_transferQueue = m_graphicsQueue;
    } else {
        poolInfo.queueFamilyIndex = (uint32_t)queueFamilies[1];
        if(vkCreateCommandPool(m_dev, &poolInfo, nullptr, &m_transferCommandPool)) {
            return RESULT_GPU_COMMAND_POOL_CREATION_FAILED;
        }
        vkGetDeviceQueue(m_dev, queueFamilies[1], 0, &m_transferQueue);
    }

    if(vkCreateCommandPool(m_dev, &poolInfo, nullptr, &m_presentCommandPool)) {
        return RESULT_GPU_COMMAND_POOL_CREATION_FAILED;
    }

    if(queueFamilies[0] == queueFamilies[2]) {
        m_presentQueue = m_graphicsQueue;
    } else if(queueFamilies[1] == queueFamilies[2]) {
        m_presentQueue = m_transferQueue;
    } if(queueFamilies[2] != queueFamilies[1] &&
           queueFamilies[2] != queueFamilies[0]) {
        vkGetDeviceQueue(m_dev, queueFamilies[2], 0, &m_presentQueue);
    }

    m_graphicsQueueIdx = queueFamilies[0];
    m_transferQueueIdx = queueFamilies[1];
    m_presentQueueIdx = queueFamilies[2];

	return RESULT_OK;
}

Result Gpu::choose_gpu(VkPhysicalDevice *pOut) {
	uint32_t numDevices = 0;
	vkEnumeratePhysicalDevices(m_instance->instance(), &numDevices, nullptr);
    if(numDevices == 0) {
        throw std::runtime_error("No GPU supporting Vulkan was found. Try installing Vulkan drivers. Remember that some GPUs does not need to support Vulkan.");
    }

	auto devices = std::vector<VkPhysicalDevice>(numDevices);
	vkEnumeratePhysicalDevices(m_instance->instance(), &numDevices, devices.data());

    VkPhysicalDevice chosen = VK_NULL_HANDLE;

    for(auto& device : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);

        uint32_t numExtensions;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, nullptr);
        std::vector<VkExtensionProperties> extensions(numExtensions);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, extensions.data());

        for(int x = 0; x < NUM_DEVICE_EXTENSIONS; x++) {
            bool isFound = false;
            for(int y = 0; y < numExtensions; y++) {
                if(!strcmp(extensions[y].extensionName, DEVICE_EXTENSIONS[x])) {
                    isFound = true;
                    break;
                }
            }

            if(!isFound) {
                lft::log::fail("Device %s does not support required device extension: (%s)",
                               props.deviceName, DEVICE_EXTENSIONS[x]);
                continue;
            }
        }

        chosen = device;
    }

    if(chosen == VK_NULL_HANDLE) {
        throw std::runtime_error("No GPU supporting all required features was found. Try updating your graphics card driver. This however might not help on older devices.");
    }

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(chosen, &props);
    lft::log::info("Selected GPU: (%s)", props.deviceName);

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
        }, {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
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


Gpu::Gpu(std::shared_ptr<const Instance> instance, std::optional<VkSurfaceKHR> supportedSurface) :
    m_instance(std::move(instance)), m_pAllocator(nullptr) {

    lft::log::warn("Warning");

	if(choose_gpu(&m_gpu)) {
		throw std::runtime_error("Failed to choose gpu");
	}

    if(create_logical_device(supportedSurface)) {
        throw std::runtime_error("Failed to create logical device");
    }

    if(create_descriptor_pool()) {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    m_pAllocator = std::make_unique<DefaultAllocator>(DefaultAllocator(this));
}

Gpu::~Gpu() {
    vkDestroyDescriptorPool(m_dev, m_descriptorPool, nullptr);
    vkDestroyCommandPool(m_dev, m_graphicsCommandPool, nullptr);
    vkDestroyCommandPool(m_dev, m_transferCommandPool, nullptr);
    vkDestroyCommandPool(m_dev, m_presentCommandPool, nullptr);
    vkDestroyDevice(m_dev, nullptr);
}

void Gpu::enqueue_present(VkPresentInfoKHR *pPresentInfo) const {
    if(vkQueuePresentKHR(m_presentQueue, pPresentInfo)) {
        throw std::runtime_error("Failed to present");
    }
}

void Gpu::enqueue_graphics(VkSubmitInfo2 *pSubmitInfo, VkFence fence) const {
    if(vkQueueSubmit2KHR(m_graphicsQueue, 1, pSubmitInfo, fence)) {
        throw std::runtime_error("Failed to submit to graphics queue");
    }
}

void Gpu::enqueue_transfer(VkSubmitInfo *pSubmitInfo, VkFence fence) const {
    vkQueueSubmit(m_transferQueue, 1, pSubmitInfo, fence);
}
