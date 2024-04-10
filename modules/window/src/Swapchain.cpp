#include "../include/Swapchain.hpp"

#include <assert.h>

VkSurfaceFormatKHR Swapchain::choose_format() {
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_pGpu->gpu(), m_surface, &formatCount, nullptr);
	assert(formatCount > 0);

	auto formats = new VkSurfaceFormatKHR[formatCount];
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_pGpu->gpu(), m_surface, &formatCount, formats);

	for(uint32_t i = 0; i < formatCount; i++) {
		if(formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
		   formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return formats[i];
		}
	}

    auto result = formats[0];

    delete [] formats;

	return result;
}

VkPresentModeKHR Swapchain::choose_present_mode() {
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_pGpu->gpu(), m_surface,
											  &presentModeCount, nullptr);

    VkPresentModeKHR presentModeResult = VK_PRESENT_MODE_FIFO_KHR;

	auto presentModes = new VkPresentModeKHR[presentModeCount];
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_pGpu->gpu(), m_surface, 
											  &presentModeCount, presentModes);

	for(uint32_t i = 0; i < presentModeCount; i++) {
		if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentModeResult = presentModes[i];
		}
	}

    delete [] presentModes;

	return presentModeResult;
}

VkExtent2D Swapchain::choose_extent() {
	VkSurfaceCapabilitiesKHR capabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_pGpu->gpu(), m_surface, &capabilities);

	if(capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		VkExtent2D actualExtent = {};
		/* actualExtent.width = MAX(
			capabilities.minImageExtent.width,
			MIN(capabilities.maxImageExtent.width, actualExtent.width)
		);

		actualExtent.height = MAX(
			capabilities.minImageExtent.height,
			MIN(capabilities.maxImageExtent.height, actualExtent.height)
		); */

		return actualExtent;	
	}
	
}

Result Swapchain::get_images(uint32_t *pOutNumImages, VkImage *pOutImages) {
	/* Get images to render to, rendertargets */
	EXPECT(vkGetSwapchainImagesKHR(m_pGpu->dev(), m_swapchain, pOutNumImages, pOutImages), "Failed to get swapchain images");
	return RESULT_OK;
}

Result Swapchain::create_views() {
	m_pImageViews = (VkImageView*)malloc(sizeof(VkImageView) * m_numImages);
	assert(m_pImageViews);

	for(unsigned i = 0; i < m_numImages; i++) {
		VkImageViewCreateInfo viewInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = m_pImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = m_format.format,
			.components = {
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		EXPECT(vkCreateImageView(m_pGpu->dev(), &viewInfo, NULL, &m_pImageViews[i]),
			   "Failed to create image view");
	}
	return RESULT_OK;
}

Result Swapchain::create_swapchain() {
	VkSurfaceCapabilitiesKHR capabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_pGpu->gpu(), m_surface, &capabilities);

	VkSwapchainCreateInfoKHR swapchainInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = m_surface,
		.minImageCount = m_numImages,
		.imageFormat = m_format.format,
		.imageColorSpace = m_format.colorSpace,
		.imageExtent = m_extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = choose_present_mode(),
		.clipped = VK_TRUE
	};

	/* Allow sharing across graphics and present queue when those two are not 
	 * the same. Meaning we can draw with commands submitted to graphics queue
	 * to swapchain images, that need to be used by command submitted to 
	 * present queue. */
    auto presentQueues = m_pGpu->present_queue_ids();
	if(presentQueues.size() > 1) {
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainInfo.queueFamilyIndexCount = presentQueues.size();
		swapchainInfo.pQueueFamilyIndices = presentQueues.data();
	} else {
		/* Don't need to share the images */
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	EXPECT(vkCreateSwapchainKHR(m_pGpu->dev(), &swapchainInfo, NULL, &m_swapchain), "Failed to create swapchain");

	return RESULT_OK;
}

Swapchain::Swapchain(Gpu *pGpu, VkSurfaceKHR surface) :
m_pGpu(pGpu), m_surface(surface) {
	VkSurfaceCapabilitiesKHR capabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_pGpu->gpu(), m_surface, &capabilities);

	m_format = choose_format();
	m_extent = choose_extent();

	uint32_t imageCount = capabilities.minImageCount + 1;
	m_numImages = imageCount;

	if(capabilities.maxImageCount > 0 &&
	   imageCount < capabilities.maxImageCount) {
		m_numImages = capabilities.maxImageCount;
	}
	m_numFrames = m_numImages;

    printf("Num images: %i\n", m_numFrames);

	create_swapchain();

	get_images(&m_numImages, NULL);
	m_pImages = new VkImage[m_numImages];
	get_images(&m_numImages, m_pImages);

	create_views();

	m_images.resize(m_numImages);
	m_views.resize(m_numImages);
	for(int i = 0; i < m_numImages; i++) {
		m_images[i].img = m_pImages[i];
		m_views[i].view = m_pImageViews[i];
	}
}
