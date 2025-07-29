#pragma once

#include <vector>

#include "props.hpp"
#include "resources/ImageView.hpp"
#include "Swapchain.hpp"

/**
 * Chain of image views. Used for swapchain and render graph output.
 */
struct ImageChain {
private:
    const std::vector<ImageView> m_images;
	const VkFormat m_format;
	const VkExtent2D m_extent;

	const VkImageLayout m_layout;

public:
	GET(m_layout, layout);
	GET(m_format, format);
	GET(m_extent, extent);

    [[nodiscard]] inline uint32_t count() const {
        return m_images.size();
    }

    [[nodiscard]] inline const std::vector<ImageView>& views() const {
        return m_images;
    }

	ImageChain(VkFormat format,
			VkExtent2D extent,
			VkImageLayout layout,
			const std::vector<ImageView>& images) :
		m_format(format),
		m_extent(extent),
		m_layout(layout),
		m_images(images) {

    }

	static ImageChain from_swapchain(const Swapchain& swapchain) {
		return ImageChain(swapchain.format().format,
				swapchain.extent(),
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				swapchain.views());
	}
};

