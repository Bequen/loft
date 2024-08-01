//
// Created by martin on 4/5/24.
//

#include "resources/Buffer.hpp"
#include "Gpu.hpp"
#include "debug/DebugUtils.h"

void Buffer::set_debug_name(const std::shared_ptr<const Gpu>& gpu, const std::string& name) const {
#if LOFT_DEBUG && VK_EXT_debug_utils
    VkDebugUtilsObjectNameInfoEXT nameInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType                    = VK_OBJECT_TYPE_BUFFER,
            .objectHandle                  = (uint64_t)buf,
            .pObjectName                   = name.c_str(),
    };

    vkSetDebugUtilsObjectNameEXT(gpu->dev(), &nameInfo);
#endif
}