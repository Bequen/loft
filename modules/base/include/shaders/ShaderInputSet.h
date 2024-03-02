#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <memory>
#include <exception>
#include <assert.h>
#include <stdexcept>

#include "resources/Buffer.hpp"
#include "resources/Image.hpp"
#include "Gpu.hpp"

/**
 * ShaderBinding
 *
 * Combines binding with storage name
 */
struct ShaderBinding {
private:
    const uint32_t m_binding;
    const std::string m_storageName;

public:
    [[nodiscard]] uint32_t binding() const { return m_binding; }
    [[nodiscard]] const std::string& storage_name() const { return m_storageName; }

    ShaderBinding(uint32_t binding, std::string storageName) :
		m_binding(binding), m_storageName(std::move(storageName)) {

    }
};

struct ShaderInputSet {
private:
    std::vector<ShaderBinding> m_bindings;

public:
    [[nodiscard]] const std::vector<ShaderBinding>& bindings() const { return m_bindings; }

    ShaderInputSet(std::vector<ShaderBinding> bindings) :
        m_bindings(std::move(bindings)){

    }
};


/**
 * ShaderInputSetBufferWrite
 * For having uniform buffer
 */
struct ShaderInputSetBufferWrite {
private:
    VkDescriptorType m_type;
    uint32_t m_binding;
    VkDescriptorBufferInfo m_bufferInfo;

public:
    /**
     * Creates new shader input set write for Buffer
     */
    ShaderInputSetBufferWrite(uint32_t binding, const Buffer& buffer,
                              uint32_t offset, uint32_t size) :
        m_bufferInfo(
                {
                    .buffer = buffer.buf,
                    .offset = offset,
                    .range = size
                }
            ),
        m_binding(binding),
        m_type(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {

    }

    const VkDescriptorBufferInfo* info() {
        return &m_bufferInfo;
    }
};

struct ShaderInputSetImageWrite {
private:
    VkDescriptorType m_type;
    uint32_t m_binding;
    VkDescriptorImageInfo m_imageInfo;

public:
    ShaderInputSetImageWrite(uint32_t binding, const ImageView& view, VkSampler sampler) :
        m_imageInfo(
                {
                    .sampler = sampler,
                    .imageView = view.view,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                }
            ),
        m_binding(binding),
        m_type(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {

    }

    const VkDescriptorImageInfo* info() {
        return &m_imageInfo;
    }
};


union ShaderInputSetWrite {
private:
    struct {
        VkDescriptorType type;
        uint32_t binding;
    } common;
    ShaderInputSetBufferWrite buffer;
    ShaderInputSetImageWrite image;

public:
    ShaderInputSetWrite() :
        common() {
    }

    explicit ShaderInputSetWrite(ShaderInputSetBufferWrite bufferWrite) :
        buffer(bufferWrite) {

    }

    explicit ShaderInputSetWrite(ShaderInputSetImageWrite imageWrite) :
        image(imageWrite) {

    }

    uint32_t is_buffer_write(VkDescriptorType type) {
        return type > VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    }

    VkWriteDescriptorSet to_write(VkDescriptorSet set) {
        return {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set,
                .dstBinding = common.binding,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = common.type,
                .pImageInfo = is_buffer_write(common.type) ? nullptr : image.info(),
                .pBufferInfo = is_buffer_write(common.type) ? buffer.info() : nullptr,
        };
    }
};


/**
 * ShaderInputSetBuilder
 *
 * Builds descriptor sets
 */
struct ShaderInputSetBuilder {
private:
    std::vector<ShaderInputSetWrite> m_writes;
    uint32_t m_numWrites;

public:
    explicit ShaderInputSetBuilder(uint32_t numWrites) :
        m_writes(numWrites), m_numWrites(0) {

    }

    ShaderInputSetBuilder& image(uint32_t binding, const ImageView& imageView, VkSampler sampler) {
        assert(imageView.view != VK_NULL_HANDLE);
        assert(binding < m_writes.size());
        assert(sampler != VK_NULL_HANDLE);

        m_writes[m_numWrites++] = ShaderInputSetWrite(ShaderInputSetImageWrite(binding, imageView, sampler));

        return *this;
    }

    ShaderInputSetBuilder& buffer(const uint32_t binding, const Buffer& buffer, const uint32_t offset, const uint32_t size) {
        assert(binding < m_writes.size());
        assert(buffer.buf != VK_NULL_HANDLE);

        m_writes[m_numWrites++] = ShaderInputSetWrite(ShaderInputSetBufferWrite(binding, buffer, offset, size));

        return *this;
    }

    /**
     * Allocates new descriptor sets from layout.
     * All the writes must be set already.
     * @param pGpu Gpu on which to perform
     * @param layout Descriptor set layout
     * @return Allocated descriptor set with all the writes.
     */
    VkDescriptorSet build(const Gpu *pGpu, VkDescriptorSetLayout layout) {
        VkDescriptorSetAllocateInfo descriptorInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = pGpu->descriptor_pool(),
                .descriptorSetCount = 1,
                .pSetLayouts = &layout
        };

        VkDescriptorSet set;
        if(vkAllocateDescriptorSets(pGpu->dev(), &descriptorInfo, &set)) {
            throw std::runtime_error("Failed to allocate descriptor set");
        }

        std::vector<VkWriteDescriptorSet> writes(m_writes.size());
        for(uint32_t i = 0; i < m_writes.size(); i++) {
            writes[i] = m_writes[i].to_write(set);
        }

        vkUpdateDescriptorSets(pGpu->dev(), writes.size(), writes.data(),
                               0, nullptr);

        return set;
    }
};
