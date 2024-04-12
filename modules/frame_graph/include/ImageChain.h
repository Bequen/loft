#pragma once

#include <vector>
#include "resources/ImageView.hpp"

struct ImageChain {
private:
    const std::vector<ImageView> m_images;

public:
    [[nodiscard]] inline uint32_t count() const {
        return m_images.size();
    }

    [[nodiscard]] inline const std::vector<ImageView>& views() const {
        return m_images;
    }

    explicit ImageChain(const std::vector<ImageView>& images) :
    m_images(images) {

    }
};