#include "resources/Image.hpp"

#include "Gpu.hpp"
#include "DebugUtils.h"

ImageView 
Image::create_view(Gpu *pGpu, VkFormat format,
				   VkImageSubresourceRange subresource) {
	VkImageViewCreateInfo viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = img,
		.viewType = subresource.layerCount == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		.format = format,
		.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange = subresource
	};

	VkImageView result = VK_NULL_HANDLE;
	vkCreateImageView(pGpu->dev(), &viewInfo, nullptr, &result);

	return {result};
}

void Image::set_debug_name(const Gpu *pGpu, const std::string& name) const {
    VkDebugUtilsObjectNameInfoEXT nameInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType                    = VK_OBJECT_TYPE_IMAGE,
            .objectHandle                  = (uint64_t)img,
            .pObjectName                   = name.c_str(),
    };

    vkSetDebugUtilsObjectName(pGpu->dev(), &nameInfo);
}