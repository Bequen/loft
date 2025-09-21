#pragma once

#include <memory>
#include <optional>

#include "Gpu.hpp"
#include "ImageChain.hpp"

void lft_dbg_callback(lft::dbg::LogMessageSeverity severity,
                      lft::dbg::LogMessageType type,
                      const char *__restrict format,
                      va_list args) {
    const char* titles[3] = {
            "\033[0;34m[info]:",
            "\033[0;33m[warn]:",
            "\033[0;31m[fail]:"
    };
    fwrite(titles[severity], 14, 1, stdout);

    if(args != nullptr) {
        vfprintf(stdout, format, args);
    } else {
        fprintf(stdout, format);
    }

    fwrite("\033[0m", 4, 1, stdout);

    printf("\n");
}

std::unique_ptr<Gpu> create_mock_gpu() {
    auto instance = std::make_unique<const Instance>(
			"loft", "loft",
			std::vector<std::string>({VK_KHR_SURFACE_EXTENSION_NAME}),
			std::vector<std::string>(),
			lft_dbg_callback);

    volkLoadInstance(instance->instance());

    return std::make_unique<Gpu>(std::move(instance), std::nullopt);
}

ImageChain create_mock_image_chain(
    const Gpu* gpu,
    uint32_t num_images,
    VkExtent2D extent,
    VkFormat format
) {
    std::vector<ImageView> images(num_images);
    for(uint32_t i = 0; i < num_images; i++) {
        MemoryAllocationInfo memory_info = {
                .usage = MEMORY_USAGE_AUTO_PREFER_DEVICE
        };

    	ImageCreateInfo image_info = {
                .extent = extent,
                .format = format,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT |
						 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .arrayLayers = 1,
                .mipLevels = 1,
        };

    	Image image = {};
    	gpu->memory()->create_image(&image_info, &memory_info, &image);

    	ImageView view = {};
    	view = image.create_view(gpu, format, {
    			.aspectMask = image_info.aspectMask,
    			.baseMipLevel = 0,
    			.levelCount = 1,
    			.baseArrayLayer = 0,
    			.layerCount = 1,
    	});

        images[i] = view;
    }

    return ImageChain(format, extent, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, images);
}
