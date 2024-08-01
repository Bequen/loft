#pragma once

#include <volk/volk.h>

#include <string>
#include <vector>
#include <memory>
#include <cstdarg>

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
    explicit Instance(const std::string& applicationName,
                      const std::string& engineName,
                      std::vector<const char*> extensions,
                      std::vector<const char*> layers,
                      lft::dbg::lft_log_callback callback);

    /**
     * Checks if the required extensions are available
     * @return A vector of indices of not available extensionse
     */
    static std::vector<uint32_t> check_extensions(const std::vector<const char*>& requiredExtensions) {
        auto supportedExtensions = list_extensions();

        std::vector<uint32_t> missingExtensions;
        for (const auto& extension : requiredExtensions) {
            auto it = std::ranges::find(supportedExtensions, extension);
            if (it == supportedExtensions.end()) {
                missingExtensions.push_back(it - supportedExtensions.begin());
            }
        }

        return missingExtensions;
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

        return std::move(devices);
    }

    /**
     * Gets a list of available instance extensions
     * @return A vector of strings representing available instance extensions
     */
    [[nodiscard]] static inline std::vector<const char*> list_extensions() {
        uint32_t numExtensions = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, nullptr);

        std::vector<VkExtensionProperties> extensions(numExtensions);
        vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, extensions.data());

        std::vector<const char*> exts;
        exts.reserve(extensions.size());
        for (const auto& extension : extensions) {
            exts.emplace_back(extension.extensionName);
        }

        return std::move(exts);
    }
};