#pragma once

#include <string>
#include <utility>
#include <vulkan/vulkan_core.h>

#include "resources/Image.hpp"
#include "resources/Buffer.hpp"

#include "props.hpp"
#include "ResourceType.h"


/*
 * Resources should only specify the layout. For the actual data, the 
 * ImageTarget and BufferTarget should be used.
 */

struct ResourceLayout {
private:
	std::string m_name;

public:
	const std::string& name() const { return m_name; }

	explicit ResourceLayout(std::string name) :
	m_name(std::move(name)) {

	}

	const virtual ResourceType resource_type() const = 0;

	const virtual bool is_similar(ResourceLayout *pResource) const = 0;

	const bool is_same_name(ResourceLayout *pResource) const {
		return m_name == pResource->name(); 
	}
};

struct AttachmentColorBlending {
    bool blendEnable;
    VkBlendFactor srcColorBlendFactor;
    VkBlendFactor dstColorBlendFactor;
    VkBlendOp colorBlendOp;

    VkBlendFactor srcAlphaBlendFactor;
    VkBlendFactor dstAlphaBlendFactor;
    VkBlendOp alphaBlendOp;
};

struct ImageResourceLayout : public ResourceLayout {
private:
	VkFormat m_format;

	VkClearValue m_clearValue = {{0.0f, 0.0f, 0.0f, 1.0f}};
	VkImageUsageFlags m_usage;
	VkImageLayout m_layout;

    /**
     * Blending
     */
    AttachmentColorBlending m_blending;

    VkAttachmentLoadOp m_loadOp;
    VkAttachmentStoreOp m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;

public:
	GET(m_format, format);
	GET(m_usage, usage);
	GET(m_clearValue, clear_value);
	GET(m_layout, layout);
    GET(m_blending, blending);
    GET(m_loadOp, load_op);
    GET(m_storeOp, store_op);

	ImageResourceLayout* set_clear_color_value(VkClearColorValue clearValue) {
		m_clearValue.color = clearValue;
		return this;
	}

	ImageResourceLayout* set_clear_depth_value(VkClearDepthStencilValue clearValue) {
		m_clearValue.depthStencil = clearValue;
		return this;
	}

	inline ImageResourceLayout* set_usage(VkImageUsageFlags usage) {
		m_usage = usage;
		return this;
	}

    inline ImageResourceLayout* set_blending(AttachmentColorBlending blending) {
        this->m_blending = blending;
        return this;
    }


    inline ImageResourceLayout* set_op(VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp) {
        this->m_loadOp = loadOp;
        this->m_storeOp = storeOp;
        return this;
    }

	ImageResourceLayout(std::string name, VkFormat format,
                        VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) :
            ResourceLayout(name), m_loadOp(VK_ATTACHMENT_LOAD_OP_CLEAR), m_storeOp(VK_ATTACHMENT_STORE_OP_STORE) {
		m_format = format;
		m_layout = layout;
	}

	const ResourceType resource_type() const override {
		return RESOURCE_TYPE_IMAGE;
	}

	const bool is_similar(ResourceLayout *pResource) const override {
		return pResource->resource_type() == resource_type() &&
               ((ImageResourceLayout*)pResource)->format() == format();
	}
};



struct BufferResourceLayout : ResourceLayout {
private:
	size_t m_offset;
	size_t m_size;

	VkBufferUsageFlags m_usage;

public:
	GET(m_usage, usage);

	inline BufferResourceLayout& set_offset(const size_t offset) {
		this->m_offset = offset;
		return *this;
	}

	inline BufferResourceLayout& set_size(const size_t size) {
		this->m_size = size;
		return *this;
	}

	inline BufferResourceLayout* set_usage(VkBufferUsageFlags usage) {
		m_usage = usage;
		return this;
	}

	inline BufferResourceLayout* set_aspect_mask(VkImageAspectFlags usage) {
		m_usage = usage;
		return this;
	}

	BufferResourceLayout(std::string name, VkBufferUsageFlags usage) : ResourceLayout(name) {
		m_usage = usage;
	}

	const ResourceType resource_type() const override {
		return RESOURCE_TYPE_BUFFER;
	}

	const virtual bool is_similar(ResourceLayout *pResource) const {
		return false;
	}
};

