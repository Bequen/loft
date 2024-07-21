#include "TextureStorage.h"

TextureStorage::~TextureStorage() {
    for(auto view : m_views) {
        vkDestroyImageView(m_gpu->dev(), view.view, nullptr);
    }

    for(auto image : m_images) {
        vkDestroyImage(m_gpu->dev(), image.img, nullptr);
    }
}