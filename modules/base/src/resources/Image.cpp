#include "resources/Image.hpp"

#include "Gpu.hpp"

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
