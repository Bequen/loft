//
// Created by martin on 3/31/24.
//

#include "Scene.h"

#include <utility>

void transfer_layout(Gpu *pGpu, const std::vector<Image>& images,
                     VkImageSubresourceRange range,
                     VkCommandBuffer stagingCommandBuffer,
                     VkImageLayout oldLayout,
                     VkImageLayout newLayout,
                     VkPipelineStageFlags srcStageMask,
                     VkPipelineStageFlags dstStageMask) {
    VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(stagingCommandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .subresourceRange = range
    };

    std::vector<VkImageMemoryBarrier> barriers(images.size(), barrier);
    for(int i = 0; i < images.size(); i++) {
        barriers[i].image = images[i].img;
    }

    vkCmdPipelineBarrier(
            stagingCommandBuffer,
            srcStageMask, dstStageMask,
            0,
            0, nullptr,
            0, nullptr,
            barriers.size(), barriers.data()
    );

    vkEndCommandBuffer(stagingCommandBuffer);

    VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &stagingCommandBuffer,
    };

    pGpu->enqueue_transfer(&submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(pGpu->transfer_queue());
}

VkCommandBuffer create_staging_command_buffer(Gpu *pGpu) {
    VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pGpu->graphics_command_pool(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(pGpu->dev(), &allocInfo, &commandBuffer);

    return commandBuffer;
}

VkDescriptorSetLayout create_layout(const std::shared_ptr<const Gpu>& gpu) {
    return ShaderInputSetLayoutBuilder(2)
            /* material buffer */
            .binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                    /* color texture */
            .binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build(gpu);
}

Scene::Scene(const std::shared_ptr<const Gpu>& gpu, const SceneData* pSceneData) :
m_gpu(gpu),
m_numTransforms(256),
m_materialBuffer(gpu, pSceneData),
m_transformBits(m_numTransforms, false),
m_bufferWriter(gpu, sizeof(Vertex) * 1024 * 256),
m_textureWriter(gpu, nullptr, {1024, 1024}, 8, 8),
m_descriptorSetLayout(create_layout(gpu)),
m_textureInputSet(VK_NULL_HANDLE){
    m_textureInputSet = ShaderInputSet(gpu, m_descriptorSetLayout);

    MemoryAllocationInfo memoryAllocationInfo = {
            .usage = MEMORY_USAGE_AUTO_PREFER_DEVICE,
            .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    };

    BufferCreateInfo materialsBufferInfo = {
            .size = sizeof(Material) * 128,
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .isExclusive = true,
    };

    materialsBufferInfo.size = sizeof(Transform) * 256;
    m_gpu->memory()->create_buffer(&materialsBufferInfo, &memoryAllocationInfo,
                                  &m_transformBuffer);
    m_transformBuffer.set_debug_name(m_gpu, "transform_buffer");

    VkSamplerCreateInfo samplerInfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0f,
            .minLod = 0.0f,
            .maxLod = (float)std::floor(std::log2(1024)) + 1,
    };

    if(vkCreateSampler(m_gpu->dev(), &samplerInfo, nullptr, &m_textureSampler)) {
        throw std::runtime_error("failed to create sampler");
    }

    m_meshBuffers.emplace_back(MeshBuffer(m_gpu, &m_bufferWriter, pSceneData->vertices(), pSceneData->indices()),
                               pSceneData->primitives());

    std::vector<VkDescriptorImageInfo> imageWriters(pSceneData->num_textures());
    for(int i = 0; i < pSceneData->num_textures(); i++) {
        imageWriters[i] = {
                .sampler = m_textureSampler,
                .imageView = m_materialBuffer.m_textureStorage.view(i).view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
    }

    ShaderInputSetWriter writer(m_gpu);
    writer.write_buffer(m_textureInputSet, 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {
            {
                    .buffer = m_materialBuffer.material_buffer()->buf,
                    .offset = 0,
                    .range = m_materialBuffer.material_buffer_size()
            }
    })
    .write_images(m_textureInputSet, 1, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageWriters)
    .write();
}

void Scene::add_scene_data(const SceneData *pData) {
    m_meshBuffers.emplace_back(MeshBuffer(m_gpu, &m_bufferWriter, pData->vertices(), pData->indices()),
                                         pData->primitives());

    // m_meshBuffers[m_meshBuffers.size() - 1].remap_materials(materialIds);

    ShaderInputSetWriter writer(m_gpu);
    writer.write_buffer(m_textureInputSet, 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {
            {
                .buffer = m_materialBuffer.material_buffer()->buf,
                .offset = 0,
                .range = m_materialBuffer.material_buffer_size()
            }
    })
    .write();
}

void Scene::draw(VkCommandBuffer cmdbuf, VkPipelineLayout layout) {
    auto descriptorSet = m_textureInputSet.descriptor_set();
    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
                            2, 1, &descriptorSet, 0, nullptr);

    for(auto& meshBuffer : m_meshBuffers) {
        size_t offsets[] = {0};
        vkCmdBindVertexBuffers(cmdbuf, 0, 1, &meshBuffer.vertex_buffer().buf, offsets);
        vkCmdBindIndexBuffer(cmdbuf, meshBuffer.index_buffer().buf, 0, VK_INDEX_TYPE_UINT32);

        for(auto& primitive : meshBuffer.primitives()) {
            uint32_t constants[2] = {0, primitive.objectIdx};
            vkCmdPushConstants(cmdbuf, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) * 2, constants);
            vkCmdDrawIndexed(cmdbuf, primitive.count, 1, primitive.offset, primitive.baseVertex, 0);
        }
    }
}

void Scene::draw_depth(VkCommandBuffer cmdbuf, VkPipelineLayout layout, mat4 transform) {
    for(auto& meshBuffer : m_meshBuffers) {
        size_t offsets[] = {0};
        vkCmdBindVertexBuffers(cmdbuf, 0, 1, &meshBuffer.vertex_buffer().buf, offsets);
        vkCmdBindIndexBuffer(cmdbuf, meshBuffer.index_buffer().buf, 0, VK_INDEX_TYPE_UINT32);

        for(auto& primitive : meshBuffer.primitives()) {
            vkCmdPushConstants(cmdbuf, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), transform);
            vkCmdDrawIndexed(cmdbuf, primitive.count, 1, primitive.offset, primitive.baseVertex, 0);
        }
    }
}