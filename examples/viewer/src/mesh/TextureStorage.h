#include <vulkan/vulkan_core.h>
#include "Gpu.hpp"
#include "resources/ImageBusWriter.h"
#include <assert.h>

typedef uint32_t TextureHandle;

class TextureStorage {
private:
    Gpu *m_pGpu;
    std::vector<uint32_t> m_textures;
    std::vector<Image> m_images;
    std::vector<ImageView> m_views;

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

        Image image;

        pGpu->memory()->create_image(&imageInfo, &memoryAllocationInfo,
                                     &image);

        return image;
    }

public:
    [[nodiscard]] const Image& image(uint32_t idx) const {
        return m_images[idx];
    }

    [[nodiscard]] const ImageView& view(uint32_t idx) const {
        return m_views[idx];
    }

    TextureStorage(Gpu *pGpu, uint32_t numTextures) :
            m_pGpu(pGpu),
            m_images(numTextures),
            m_views(numTextures),
            m_textures(numTextures, false),
            m_writer(pGpu, nullptr, {2048, 2048}, 8, 1)
    {
        assert(pGpu != nullptr);
        assert(numTextures > 0);
    }

    TextureHandle pop_handle() {
        TextureHandle handle = m_idx;
        m_textures[handle] = true;
        m_idx++;

        return handle;
    }

    /**
     * Pushes new image into the storage and returns it's handle
     * @param pData Data to be send
     * @param width Width of the image
     * @param height Height of the image
     * @return Returns handle to the newly added image
     */
    TextureHandle upload(unsigned char* pData, uint32_t width, uint32_t height, size_t size, VkFormat format) {
        TextureHandle handle = pop_handle();

        VkBufferImageCopy region = {
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                },
                .imageOffset = { 0, 0, 0 },
                .imageExtent = {
                        (uint32_t)width, (uint32_t)height, 1
                }
        };

        ImageCreateInfo imageCreateInfo = {
                .format = format,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                         VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .arrayLayers = 1,
                .mipLevels = 1,
                .extent = {
                        width, height
                }
        };

        MemoryAllocationInfo memoryInfo = {
                .usage = MEMORY_USAGE_AUTO_PREFER_DEVICE
        };

        m_pGpu->memory()->create_image(&imageCreateInfo, &memoryInfo, &m_images[handle]);
        m_views[handle] = m_images[handle].create_view(m_pGpu, format, {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseArrayLayer = 0,
            .baseMipLevel = 0,
            .layerCount = 1,
            .levelCount = 1
        });

        m_writer.set_target(&m_images[handle]);
        m_writer.write(region, pData, size);
        m_writer.flush();

        return handle;
    }

    void unload(TextureHandle handle) {

    }
};
