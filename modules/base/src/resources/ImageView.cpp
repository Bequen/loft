#include "resources/ImageView.hpp"


#include "Gpu.hpp"
#include "DebugUtils.h"

void ImageView::set_debug_name(const Gpu *pGpu, const std::string &name) const {
    VkDebugUtilsObjectNameInfoEXT nameInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType                    = VK_OBJECT_TYPE_IMAGE_VIEW,
            .objectHandle                  = (uint64_t)view,
            .pObjectName                   = name.c_str(),
    };

    vkSetDebugUtilsObjectName(pGpu->dev(), &nameInfo);
}

