#include <vulkan/vulkan_core.h>
#include "Gpu.hpp"
#include "resources/ImageBusWriter.h"
#include <assert.h>

typedef uint32_t TextureHandle;

class TextureStorage {
private:
    Gpu *m_pGpu;
    std::vector<uint32_t> m_textures;

    Image m_image;
    ImageView m_view;

    TextureHandle m_idx = 0;

    ImageBusWriter m_writer;

    Image create_image(Gpu *pGpu, VkExtent2D extent, VkFormat format, uint32_t arrayLayers, uint32_t mipLevels) {
        MemoryAllocationInfo memoryAllocationInfo = {
                .usage = MEMORY_USAGE_AUTO_PREFER_DEVICE
        };

        ImageCreateInfo imageInfo = {
                .extent = extent,
                .format = format,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                         VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .arrayLayers = arrayLayers,
                .mipLevels = mipLevels
        };

        pGpu->memory()->create_image(&imageInfo, &memoryAllocationInfo,
                                     &m_image);

        return m_image;
    }

public:
    [[nodiscard]] const Image& image() const {
        return m_image;
    }

    const ImageView& view() const {
        return m_view;
    }

    TextureStorage(Gpu *pGpu, VkExtent2D extent, VkFormat format, uint32_t arrayLayers, uint32_t mipLevels) :
            m_pGpu(pGpu),
            m_image(create_image(pGpu, extent, format, arrayLayers, mipLevels)),
            m_view(m_image.create_view(pGpu, format, VkImageSubresourceRange {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = mipLevels,
                .baseArrayLayer = 0,
                .layerCount = arrayLayers,
            })),
            m_writer(pGpu, &m_image, extent, 8, 1),
            m_textures(arrayLayers, false) {
        assert(pGpu != nullptr);
        assert(format != 0);
        assert(extent.width > 0 && extent.height > 0);
        assert(arrayLayers > 1);
    }

    TextureHandle find_free_handle() {
        return m_idx++;
    }

    TextureHandle upload(unsigned char* pData, uint32_t width, uint32_t height, uint32_t perPixelSize) {
        TextureHandle handle = find_free_handle();

        VkBufferImageCopy region = {
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = (uint32_t)handle,
                        .layerCount = 1,
                },
                .imageOffset = { 0, 0, 0 },
                .imageExtent = {
                        (uint32_t)width, (uint32_t)height, 1
                }
        };

        m_writer.write(region, pData, width * height * perPixelSize);

        m_textures[handle] = true;

        return handle;
    }

    void unload(TextureHandle handle) {

    }
};