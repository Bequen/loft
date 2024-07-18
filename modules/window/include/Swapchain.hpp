#pragma once

#include "Gpu.hpp"
#include "Window.hpp"
#include "props.hpp"
#include "resources/Image.hpp"

#include <vector>
#include <volk/volk.h>
#include <stdexcept>

struct ImageResourceLayout;

class Swapchain {
private:
	std::shared_ptr<const Gpu> m_gpu;
	VkSurfaceKHR m_surface;
	VkSwapchainKHR m_swapchain;

	VkSurfaceFormatKHR m_format;
	VkExtent2D m_extent;

	unsigned m_numImages;
	unsigned m_numFrames;

	VkImage *m_pImages;
	VkImageView *m_pImageViews;

	VkSurfaceFormatKHR choose_format();
	VkPresentModeKHR choose_present_mode();
	VkExtent2D choose_extent();

	Result create_swapchain();

	Result get_images(unsigned *pOutCount, VkImage *pOut);

	Result create_views();

	std::vector<Image> m_images;
	std::vector<ImageView> m_views;

public:
	GET(m_format, format);
	GET(m_swapchain, swapchain);
	GET(m_extent, extent);
	GET(m_numImages, num_images);
	REF(m_images, images);
	REF(m_views, views);
    GET(m_surface, surface);

	const Image* get_image(uint32_t idx) const {
		if(idx >= m_numImages) {
			return nullptr;
		}

		return &m_images[idx];
	}

    VkResult get_next_image_idx(VkSemaphore signalSemaphore, VkFence signalFence, uint32_t *pOutImageIdx) const {
        return vkAcquireNextImageKHR(m_gpu->dev(), m_swapchain, UINT64_MAX, signalSemaphore, signalFence, pOutImageIdx);
    }

    void present(std::vector<VkSemaphore> waitSemaphores,
                 uint32_t imageIdx) {
        VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.size();
		presentInfo.pWaitSemaphores = waitSemaphores.data();
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_swapchain;
		presentInfo.pImageIndices = &imageIdx;

        m_gpu->enqueue_present(&presentInfo);
    }

	Swapchain(const std::shared_ptr<const Gpu>& gpu, VkSurfaceKHR surface);
};
