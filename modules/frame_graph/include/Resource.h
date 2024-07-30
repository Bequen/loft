#pragma once

#include <volk/volk.h>
#include <utility>
#include <vector>
#include "ResourceType.h"
#include "resources/Image.hpp"
#include "resources/Buffer.hpp"

struct ImageResource;

/**
 * Represents a resource in render graph.
 */
struct Resource {
    virtual const ResourceType type() const = 0;

    ImageResource* as_image() {
        return (ImageResource*)this;
    }
};

struct ImageResource : public Resource {
    Image image;
    ImageView view;
    uint32_t flags;

    ImageResource(Image image, ImageView view, bool isDepth = false) :
    image(image), view(view), flags(isDepth) {

    }

    [[nodiscard]] const ResourceType type() const override {
        return RESOURCE_TYPE_IMAGE;
    }

    bool is_depth() const {
        return flags;
    }
};

struct BufferResource : public Resource {
    Buffer buffer;

    BufferResource(Buffer buffer) :
    buffer(std::move(buffer)) {

    }

    [[nodiscard]] const ResourceType type() const override {
        return RESOURCE_TYPE_BUFFER;
    }
};



/**
 *
 */
/* struct ImageResource : public Resource {
private:
    std::vector<Image> m_images;
    std::vector<ImageView> m_views;

    VkSampler m_sampler;

public:
    ImageResource(const std::vector<Image>& images,
                  const std::vector<ImageView>& views,
                  VkSampler sampler) :
                  m_images(images), m_views(views), m_sampler(sampler) {

    }

	const ImageView& view(uint32_t frame) {
		return m_views[frame];
	}

	[[nodiscard]] inline uint32_t num_frames() const {
		return m_views.size();
	}

    [[nodiscard]] inline const ResourceType resource_type() const override {
        return RESOURCE_TYPE_IMAGE;
    }

    [[nodiscard]] inline const VkSampler sampler() const {
        return m_sampler;
    }
}; */
