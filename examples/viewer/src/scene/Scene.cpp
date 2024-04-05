//
// Created by martin on 3/31/24.
//

#include "Scene.h"

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

Scene::Scene(Gpu *pGpu) :
m_pGpu(pGpu),
m_colorTextures(pGpu, {2048, 2048}, VK_FORMAT_R8G8B8A8_SRGB, 32, 8),
m_normalTextures(pGpu, {2048, 2048}, VK_FORMAT_R8G8B8A8_SRGB, 32, 8),
m_pbrTextures(pGpu, {2048, 2048}, VK_FORMAT_R8G8B8A8_SRGB, 32, 8),
m_numMaterials(128),
m_numTransforms(256),
m_materialsBits(m_numMaterials, false),
m_transformBits(m_numTransforms, false),
m_bufferWriter(pGpu, nullptr, sizeof(Vertex) * 1024),
m_textureWriter(pGpu, nullptr, {2048, 2048}, 4, 8) {

    m_colorTextures.image().set_debug_name(pGpu, "color_texture_storage");
    m_colorTextures.view().set_debug_name(pGpu, "color_texture_storage_view");

    m_normalTextures.image().set_debug_name(pGpu, "normal_texture_storage");
    m_normalTextures.view().set_debug_name(pGpu, "normal_texture_storage_view");

    m_pbrTextures.image().set_debug_name(pGpu, "pbr_texture_storage");
    m_pbrTextures.view().set_debug_name(pGpu, "pbr_texture_storage_view");

    MemoryAllocationInfo memoryAllocationInfo = {
            .usage = MEMORY_USAGE_AUTO_PREFER_DEVICE,
            .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    };

    BufferCreateInfo materialsBufferInfo = {
            .size = sizeof(Material) * 128,
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .isExclusive = true,
    };
    pGpu->memory()->create_buffer(&materialsBufferInfo, &memoryAllocationInfo,
                                  &m_materialBuffer);
    m_materialBuffer.set_debug_name(m_pGpu, "material_buffer");

    materialsBufferInfo.size = sizeof(Transform) * 256;
    pGpu->memory()->create_buffer(&materialsBufferInfo, &memoryAllocationInfo,
                                  &m_transformBuffer);
    m_transformBuffer.set_debug_name(m_pGpu, "transform_buffer");

    VkCommandBuffer cmdbuf = create_staging_command_buffer(pGpu);
    transfer_layout(pGpu,
                    {m_colorTextures.image(), m_normalTextures.image(), m_pbrTextures.image()},
                    (VkImageSubresourceRange) {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 8,
                        .baseArrayLayer = 0,
                        .layerCount = 32,
                    },
                    cmdbuf,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT);

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
            .maxLod = (float)8,
    };

    if(vkCreateSampler(pGpu->dev(), &samplerInfo, nullptr, &m_textureSampler)) {
        throw std::runtime_error("failed to create sampler");
    }
}

uint32_t Scene::push_material(const MaterialData& materialData) {
    auto found = std::ranges::find(m_materialsBits, false);
    auto distance = std::distance(m_materialsBits.begin(), found);

    Material material;

    char *pData;
    m_pGpu->memory()->map(m_materialBuffer.allocation, (void**)&pData);
    memcpy(pData + sizeof(Material) * distance, &material, sizeof(Material));
    m_pGpu->memory()->unmap(m_materialBuffer.allocation);

    return distance;
}

std::vector<uint32_t> Scene::load_materials(const SceneData *pData) {
    std::vector<uint32_t> materials(pData->materials().size());
    uint32_t i = 0;
    for(auto& material : pData->materials()) {
        materials[i++] = push_material(material);
    }

    return materials;
}


void Scene::pop_material(uint32_t materialIdx) {
    m_materialsBits[materialIdx] = false;
}

void Scene::add_scene_data(const SceneData *pData) {
    m_meshBuffers.emplace_back(MeshBuffer(m_pGpu, &m_bufferWriter, pData->vertices(), pData->indices()),
                               pData->primitives());

    auto materialIds = load_materials(pData);
}

void Scene::draw(VkCommandBuffer cmdbuf, VkPipelineLayout layout) {
    for(auto& meshBuffer : m_meshBuffers) {
        size_t offsets[] = {0};
        vkCmdBindVertexBuffers(cmdbuf, 0, 1, &meshBuffer.vertex_buffer().buf, offsets);
        vkCmdBindIndexBuffer(cmdbuf, meshBuffer.index_buffer().buf, 0, VK_INDEX_TYPE_UINT32);

        for(auto& primitive : meshBuffer.primitives()) {
            uint32_t constants[2] = {0, primitive.objectIdx};
            vkCmdPushConstants(cmdbuf, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), constants);
            vkCmdDrawIndexed(cmdbuf, primitive.count, 1, primitive.offset, primitive.baseVertex, 0);
        }
    }
}

