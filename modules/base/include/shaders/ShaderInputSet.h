#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <volk/volk.h>
#include <memory>
#include <exception>
#include <assert.h>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#include "resources/Buffer.hpp"
#include "resources/Image.hpp"
#include "Gpu.hpp"


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
    ShaderInputSetBufferWrite(VkDescriptorType type, uint32_t binding, const Buffer& buffer,
                              uint32_t offset, uint32_t size) :
        m_bufferInfo(
                {
                    .buffer = buffer.buf,
                    .offset = offset,
                    .range = size
                }
            ),
        m_binding(binding),
        m_type(type) {

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


#include <iostream>
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

    ShaderInputSetBuilder& buffer(const VkDescriptorType type, const uint32_t binding, const Buffer& buffer, const uint32_t offset, const uint32_t size) {
        assert(binding < m_writes.size());
        assert(buffer.buf != VK_NULL_HANDLE);

        m_writes[m_numWrites++] = ShaderInputSetWrite(ShaderInputSetBufferWrite(type, binding, buffer, offset, size));

        return *this;
    }

    /**
     * Allocates new descriptor sets from layout.
     * All the writes must be set already.
     * @param pGpu Gpu on which to perform
     * @param layout Descriptor set layout
     * @return Allocated descriptor set with all the writes.
     */
    VkDescriptorSet build(const std::shared_ptr<const Gpu>& gpu, VkDescriptorSetLayout layout) {
        VkDescriptorSetAllocateInfo descriptorInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = gpu->descriptor_pool(),
                .descriptorSetCount = 1,
                .pSetLayouts = &layout
        };

        VkDescriptorSet set;
        if(vkAllocateDescriptorSets(gpu->dev(), &descriptorInfo, &set)) {
            throw std::runtime_error("Failed to allocate descriptor set");
        }

        std::vector<VkWriteDescriptorSet> writes(m_writes.size());
        for(uint32_t i = 0; i < m_writes.size(); i++) {
            writes[i] = m_writes[i].to_write(set);
        }

        vkUpdateDescriptorSets(gpu->dev(), writes.size(), writes.data(),
                               0, nullptr);

        return set;
    }
};

struct ShaderInputSet {
private:
    VkDescriptorSet m_descriptorSet;

public:
    inline const VkDescriptorSet descriptor_set() const {
        return m_descriptorSet;
    }

    ShaderInputSet(VkDescriptorSet descriptorSet) :
        m_descriptorSet(descriptorSet) {

    }

    ShaderInputSet(const std::shared_ptr<const Gpu>& gpu, VkDescriptorSetLayout layout) :
        m_descriptorSet(VK_NULL_HANDLE) {

        VkDescriptorSetAllocateInfo descriptorInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = gpu->descriptor_pool(),
                .descriptorSetCount = 1,
                .pSetLayouts = &layout
        };

        if(vkAllocateDescriptorSets(gpu->dev(), &descriptorInfo, &m_descriptorSet)) {
            throw std::runtime_error("Failed to allocate descriptor set");
        }
    }
};

enum ShaderInputWriteType {

};

class ShaderInputSetWriter {
private:
    std::shared_ptr<const Gpu> m_gpu;
    std::vector<VkWriteDescriptorSet> m_writes;

    std::vector<std::vector<VkDescriptorImageInfo>> m_imageWrites;
    std::vector<std::vector<VkDescriptorBufferInfo>> m_bufferWrites;

public:
    explicit ShaderInputSetWriter(const std::shared_ptr<const Gpu>& gpu) :
            m_gpu(gpu) {

    }

    ShaderInputSetWriter& write_images(const ShaderInputSet& dstInputSet,
                                       const uint32_t dstBinding,
                                       const uint32_t dstArrayElement,
                                       const VkDescriptorType type,
                                       const std::vector<VkDescriptorImageInfo>& writes) {
        /* copy write data, to not lose it */
        m_imageWrites.push_back(writes);

        m_writes.push_back({
               .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
               .dstSet = dstInputSet.descriptor_set(),
               .dstBinding = dstBinding,
               .dstArrayElement = dstArrayElement,
               .descriptorCount = (uint32_t)writes.size(),
               .descriptorType = type,
               .pImageInfo = m_imageWrites[m_imageWrites.size() - 1].data()
        });

        return *this;
    }

    ShaderInputSetWriter& write_buffer(const ShaderInputSet& dstInputSet,
                                       const uint32_t dstBinding,
                                       const uint32_t dstArrayElement,
                                       const VkDescriptorType type,
                                       const std::vector<VkDescriptorBufferInfo>& writes) {
        /* copy write data, to not lose it */
        m_bufferWrites.push_back(writes);

        m_writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = dstInputSet.descriptor_set(),
                .dstBinding = dstBinding,
                .dstArrayElement = dstArrayElement,
                .descriptorCount = (uint32_t)writes.size(),
                .descriptorType = type,
                .pBufferInfo = m_bufferWrites[m_bufferWrites.size() - 1].data()
        });

        return *this;
    }

    ShaderInputSetWriter& write() {
        vkUpdateDescriptorSets(m_gpu->dev(), m_writes.size(), m_writes.data(), 0, nullptr);
        return *this;
    }
};
