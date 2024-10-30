#include <volk/volk.h>
#include <cstring>
#include <stdexcept>

#include "Instance.hpp"
#include "result.hpp"
#include "debug/Debug.hpp"


#if LOFT_DEBUG && VK_LAYERS_ENABLE
static const char* LAYERS[] = {
        "VK_LAYER_KHRONOS_validation"
};

static const int NUM_LAYERS = sizeof(LAYERS) / sizeof(*LAYERS);
#else
#define LAYERS nullptr
static const int NUM_LAYERS = 0;
#endif

static const char* EXTENSIONS[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
};

static const int NUM_EXTENSIONS = sizeof(EXTENSIONS) / sizeof(*EXTENSIONS);


static const char *DEVICE_EXTENSIONS[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

static const int NUM_DEVICE_EXTENSIONS = sizeof(DEVICE_EXTENSIONS) / sizeof(*DEVICE_EXTENSIONS);

static bool IS_INITIALIZED = false;

lft::dbg::lft_log_callback g_logCallback = nullptr;

VkBool32
vk_dbg_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                VkDebugUtilsMessageTypeFlagsEXT type,
                const VkDebugUtilsMessengerCallbackDataEXT* pData,
                void *pUserData) {

    g_logCallback(lft::dbg::LogMessageSeverity::error, lft::dbg::LogMessageType::general, pData->pMessage, {});

    return VK_FALSE;
}

Instance::Instance(const std::string& applicationName,
                   const std::string& engineName,
                   std::vector<const char*> extensions,
                   std::vector<const char*> layers,
                   lft::dbg::lft_log_callback callback) {
    // check unsupported extensions
    auto unsupportedExtensions = check_extensions(extensions);
    EXPECT(!unsupportedExtensions.empty(), "Unsupported extensions");

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = strdup(applicationName.c_str()),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = strdup(engineName.c_str()),
        .apiVersion = VK_API_VERSION_1_1
    };

    VkDebugUtilsMessengerCreateInfoEXT dbgInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = vk_dbg_callback,
        .pUserData = nullptr
    };

    // collect extensions
    char** pExtensions = new char*[extensions.size() + NUM_EXTENSIONS];
    uint32_t i = 0;
    for(; i < extensions.size(); i++) {
        pExtensions[i] = strdup(extensions[i]);
    }
    memcpy(pExtensions + i, EXTENSIONS, sizeof(char*) * NUM_EXTENSIONS);
    i += NUM_EXTENSIONS;

    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#if LOFT_DEBUG
        .pNext = &dbgInfo,
#endif
        .pApplicationInfo = &appInfo,
#if LOFT_DEBUG
        .enabledLayerCount = NUM_LAYERS,
        .ppEnabledLayerNames = LAYERS,
#endif
        .enabledExtensionCount = i,
        .ppEnabledExtensionNames = pExtensions,
    };

    if(callback != nullptr) {
        g_logCallback = callback;
    }

    EXPECT(vkCreateInstance(&instanceInfo, nullptr, &m_instance) == VK_SUCCESS,
           "Failed to create vulkan instance");

    lft::log::warn("Instance created successfully");

    // cleanup
    for(; i > 0; i--) {
        // free(pExtensions[i]);
    }
    delete [] pExtensions;

    IS_INITIALIZED = true;
}