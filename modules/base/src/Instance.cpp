#include <volk/volk.h>
#include <signal.h>
#include <string>
#include <vector>
#include <cstring>

#include "Instance.hpp"
#include "result.hpp"

static const char* EXTENSIONS[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
};

static const int NUM_EXTENSIONS = sizeof(EXTENSIONS) / sizeof(*EXTENSIONS);

#if VK_LAYERS_ENABLE
static const char* LAYERS[] = {
        "VK_LAYER_KHRONOS_validation"
};

static const int NUM_LAYERS = sizeof(LAYERS) / sizeof(*LAYERS);
#else
#define LAYERS nullptr

static const int NUM_LAYERS = 0;
#endif

static const char *DEVICE_EXTENSIONS[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

static const int NUM_DEVICE_EXTENSIONS = sizeof(DEVICE_EXTENSIONS) / sizeof(*DEVICE_EXTENSIONS);

#define VULKAN_SIGTRAP_ON_ERROR 1

static bool IS_INITIALIZED = false;

VkBool32
vk_dbg_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                VkDebugUtilsMessageTypeFlagsEXT type,
                const VkDebugUtilsMessengerCallbackDataEXT* pData,
                void *pUserData) {
    printf("\n[ERROR]: %s", pData->pMessageIdName);
    printf("%s\n", pData->pMessage);
    printf("\n");

#if VULKAN_SIGTRAP_ON_ERROR
    if(IS_INITIALIZED && (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ||
                          severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)) {
        /// raise(SIGTRAP);
    }
#endif

    return VK_FALSE;
}

Instance::Instance(std::string applicationName, std::string engineName,
                   uint32_t numExtensions, char** extensions) {
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "applicationName.c_str()",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "engineName.c_str()",
        .apiVersion = VK_API_VERSION_1_1
    };

    VkDebugUtilsMessengerCreateInfoEXT dbgInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = vk_dbg_callback,
        .pUserData = nullptr
    };

    char** pExtensions = new char*[numExtensions + NUM_EXTENSIONS];
    memcpy(pExtensions, extensions, sizeof(char*) * numExtensions);
    memcpy(pExtensions + numExtensions, EXTENSIONS, sizeof(char*) * NUM_EXTENSIONS);

    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &dbgInfo,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = NUM_LAYERS,
        .ppEnabledLayerNames = LAYERS,
        .enabledExtensionCount = NUM_EXTENSIONS + numExtensions,
        .ppEnabledExtensionNames = pExtensions,
    };
    EXPECT(vkCreateInstance(&instanceInfo, nullptr, &m_instance),
           "Failed to create vulkan instance");
    
    IS_INITIALIZED = true;
}