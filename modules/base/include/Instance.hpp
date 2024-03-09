#pragma once

#include <vulkan/vulkan_core.h>
#include <string>
#include <vector>
#include "props.hpp"


/**
 * Instance
 */
class Instance {
private:
    VkInstance m_instance;

public:
    GET(m_instance, instance);

    explicit Instance(std::string applicationName,
                      std::string engineName,
                      uint32_t numExtensions,
                      char** extensions);
};
