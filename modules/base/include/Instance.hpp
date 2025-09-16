#pragma once

#include <iterator>
#include <volk.h>

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <vulkan/vulkan_core.h>

#include "props.hpp"
#include "debug/Debug.hpp"

/**
 * Maintains a connection to a GPU driver
 */
class Instance {
private:
    VkInstance m_instance;

public:
    GET(m_instance, instance);

    // disable copy
    Instance(const Instance&) = delete;
    Instance() = delete;

    /**
     * Creates new instance and initializes a connection to a GPU driver
     * @param applicationName Name of the application
     * @param engineName Name of the rendering engine
     * @param extensions Additional Vulkan extensions you might want to enable
     * @param callback Debug log callback
     */
    Instance(const std::string applicationName,
            const std::string engineName,
            std::vector<std::string> extensions,
            std::vector<std::string> layers,
            lft::dbg::lft_log_callback callback);

    static std::vector<std::string> find_unsupported_layers(
            std::vector<std::string>& required_layers
    ) {
        uint32_t available_layers_count = 0;
        vkEnumerateInstanceLayerProperties(&available_layers_count, nullptr);

        if(available_layers_count == 0) {
            return required_layers;
        }

        std::vector<VkLayerProperties> available_layers(available_layers_count);
        vkEnumerateInstanceLayerProperties(&available_layers_count, available_layers.data());

        std::vector<std::string> available_layer_names(available_layers_count);
        std::transform(available_layers.begin(), available_layers.end(),
                available_layer_names.begin(), 
                [](const VkLayerProperties& props) {
                    std::cout << "Supported: [" << props.layerName << "]" << std::endl;
                    return std::string(props.layerName);
                });

        std::sort(available_layer_names.begin(), available_layer_names.end());
        std::sort(required_layers.begin(), required_layers.end());

        for(auto layer : required_layers) {
            std::cout << "Required: [" << layer << "]" << std::endl;
        }

        std::vector<std::string> unsupported_layers;
        std::set_difference(required_layers.begin(), required_layers.end(),
                available_layer_names.begin(), available_layer_names.end(),
                std::back_inserter(unsupported_layers));

        return unsupported_layers;
    }

    /**
     * Checks if the required extensions are available
     * @return A vector of indices of not available extensionse
     */
    static std::vector<std::string> check_extensions(const std::vector<std::string>& requiredExtensions) {
        auto supportedExtensions = list_extensions();

        std::vector<std::string> unsupported_extensions;
        std::set_difference(requiredExtensions.begin(), requiredExtensions.end(),
                supportedExtensions.begin(), supportedExtensions.end(),
                std::back_inserter(unsupported_extensions));

        return unsupported_extensions;
    }


    /**
     * Gets a list of available physical devices
     * @return A vector of VkPhysicalDevice objects representing available physical devices
     */
    [[nodiscard]] inline std::vector<VkPhysicalDevice> list_physical_devices() const {
        uint32_t numDevices = 0;
        vkEnumeratePhysicalDevices(m_instance, &numDevices, nullptr);

        std::vector<VkPhysicalDevice> devices(numDevices);
        vkEnumeratePhysicalDevices(m_instance, &numDevices, devices.data());

        return devices;
    }

    /**
     * Gets a list of available instance extensions
     * @return A vector of strings representing available instance extensions
     */
    [[nodiscard]] static inline std::vector<std::string> list_extensions() {
        uint32_t numExtensions = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, nullptr);

        std::vector<VkExtensionProperties> extensions(numExtensions);
        vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, extensions.data());

        std::vector<std::string> extension_names(numExtensions);
        std::transform(extensions.begin(), extensions.end(),
                extension_names.begin(),
                [](const VkExtensionProperties& props) {
                    return std::string(props.extensionName);
                });

        return extension_names;
    }
};
