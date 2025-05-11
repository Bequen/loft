#pragma once

#include <volk/volk.h>

class ImageResource {
public:
	VkImage image;
	VkImageView image_view;

	ImageResource(VkImage image, VkImageView image_view) :
		image(image),
		image_view(image_view) {
	};
};
