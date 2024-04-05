//
// Created by martin on 4/5/24.
//

#include "resources/Buffer.hpp"
#include "Gpu.hpp"
#include "DebugUtils.h"

void Buffer::set_debug_name(const Gpu *pGpu, const std::string& name) const {
    VkDebugUtilsObjectNameInfoEXT nameInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType                    = VK_OBJECT_TYPE_BUFFER,
            .objectHandle                  = (uint64_t)buf,
            .pObjectName                   = name.c_str(),
    };

    vkSetDebugUtilsObjectName(pGpu->dev(), &nameInfo);
}