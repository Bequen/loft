#include <iostream>
#include <numeric>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <iterator>

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
		VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
};

static const int NUM_DEVICE_EXTENSIONS = sizeof(DEVICE_EXTENSIONS) / sizeof(*DEVICE_EXTENSIONS);

static bool IS_INITIALIZED = false;

lft::dbg::lft_log_callback g_logCallback = nullptr;

VkBool32
vk_dbg_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                VkDebugUtilsMessageTypeFlagsEXT type,
                const VkDebugUtilsMessengerCallbackDataEXT* pData,
                void *pUserData) {

    if(g_logCallback != nullptr) {
        g_logCallback(lft::dbg::LogMessageSeverity::error,
            lft::dbg::LogMessageType::general,
            pData->pMessage, {});
    } else {
        std::cout << pData->pMessage << std::endl;
    }

    return VK_FALSE;
}

std::string get_unsupported_layers_error_mesg(std::vector<std::string> unsupported_layers) {
    std::stringstream str;
    std::copy(unsupported_layers.begin(), unsupported_layers.end(),
            std::ostream_iterator<std::string>(str, "\n"));
    return "Unsupported layers: \n" + str.str();
}

std::string get_unsupported_extensions_error_mesg(std::vector<std::string> unsupported_extensions) {
    std::stringstream str;
    std::copy(unsupported_extensions.begin(), unsupported_extensions.end(),
            std::ostream_iterator<std::string>(str, "\n"));
    return "Unsupported extensions: \n" + str.str();
}

std::vector<char*> transform_strings_to_c_strings(std::vector<std::string> strings) {
    std::vector<char*> result(strings.size());
    std::transform(strings.begin(), strings.end(),
            result.begin(),
            [](const std::string& str) {
                return strndup(str.c_str(), str.length());
            });

    return result;
}

Instance::Instance(const std::string applicationName,
                   const std::string engineName,
                   std::vector<std::string> extensions,
                   std::vector<std::string> layers,
                   lft::dbg::lft_log_callback callback) {
    if(volkInitialize()) {
        throw std::runtime_error("Failed to initialize volk");
    }

    // check unsupported extensions
    auto unsupportedExtensions = check_extensions(extensions);
    auto unsupportedLayers = find_unsupported_layers(layers);
    if(!unsupportedExtensions.empty()) {
        throw std::runtime_error(get_unsupported_extensions_error_mesg(unsupportedExtensions));
    }

    if(!unsupportedLayers.empty()) {
        throw std::runtime_error(get_unsupported_layers_error_mesg(unsupportedLayers));
    }
    // EXPECT(!unsupportedExtensions.empty(), "Unsupported extensions");

    if(callback) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "test", // strdup(applicationName.c_str()),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "test",// strdup(engineName.c_str()),
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

    std::vector<char*> extension_cstrs = transform_strings_to_c_strings(extensions);
    std::vector<char*> layer_cstrs = transform_strings_to_c_strings(layers);

    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#if LOFT_DEBUG
        .pNext = &dbgInfo,
#endif
        .pApplicationInfo = &appInfo,
#if LOFT_DEBUG
        .enabledLayerCount = (uint32_t)layer_cstrs.size(),
        .ppEnabledLayerNames = layer_cstrs.data(),
#endif
        .enabledExtensionCount = (uint32_t)extension_cstrs.size(),
        .ppEnabledExtensionNames = extension_cstrs.data(),
    };

    if(callback != nullptr) {
        g_logCallback = callback;
    }

    EXPECT(vkCreateInstance(&instanceInfo, nullptr, &m_instance) == VK_SUCCESS,
           "Failed to create vulkan instance");

    volkLoadInstance(m_instance);

    lft::log::warn("Instance created successfully");

    // cleanup
    for(char* str : extension_cstrs) {
        delete [] str;
    }

    for(char* str : layer_cstrs) {
        delete [] str;
    }

    IS_INITIALIZED = true;
}
