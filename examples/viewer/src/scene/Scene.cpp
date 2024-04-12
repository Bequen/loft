//
// Created by martin on 3/31/24.
//

#include "Scene.h"

#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

VkDescriptorSetLayout create_layout(Gpu *pGpu) {
    return ShaderInputSetLayoutBuilder(2)
            /* material buffer */
            .binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                    /* color texture */
            .binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build(pGpu);
}

Scene::Scene(Gpu *pGpu) :
m_pGpu(pGpu),
m_colorTextures(pGpu, 128),
m_normalTextures(pGpu, 64),
m_pbrTextures(pGpu, 64),
m_numMaterials(128),
m_numTransforms(256),
m_materialsBits(m_numMaterials, false),
m_transformBits(m_numTransforms, false),
m_bufferWriter(pGpu, sizeof(Vertex) * 1024 * 256),
m_textureWriter(pGpu, nullptr, {1024, 1024}, 8, 8),
m_descriptorSetLayout(create_layout(pGpu)),
m_textureInputSet(VK_NULL_HANDLE) {
    m_textureInputSet = ShaderInputSet(pGpu, m_descriptorSetLayout);

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

    if(vkCreateSampler(pGpu->dev(), &samplerInfo, nullptr, &m_textureSampler)) {
        throw std::runtime_error("failed to create sampler");
    }
}

unsigned char *load_image_data(const std::string& path, int32_t *pOutWidth, int32_t *pOutHeight, int32_t *pOutNumChannels, int32_t numChannels) {
    unsigned char *data = stbi_load(path.c_str(), pOutWidth, pOutHeight, pOutNumChannels, numChannels);

    return data;
}

unsigned short *load_image_data_16(const std::string& path, int32_t *pOutWidth, int32_t *pOutHeight, int32_t *pOutNumChannels, int32_t numChannels) {
    unsigned short *data = stbi_load_16(path.c_str(), pOutWidth, pOutHeight, pOutNumChannels, numChannels);

    return data;
}

uint32_t Scene::push_material(const SceneData *pSceneData, const MaterialData& materialData) {
    auto found = std::ranges::find(m_materialsBits, false);
    auto distance = std::distance(m_materialsBits.begin(), found);

    Material material = {};
    memcpy(material.albedo, materialData.albedo(), sizeof(vec4));
    memcpy(material.bsdf, materialData.bsdf(), sizeof(vec4));

    ShaderInputSetWriter writer(m_pGpu);

    if(materialData.color_texture().has_value()) {
        int32_t width, height, numChannels;
        unsigned char* pImageData = load_image_data(pSceneData->textures()[materialData.color_texture().value()].m_path,
                                                    &width, &height, &numChannels, 4);

        material.colorTexture = m_colorTextures.upload((unsigned char*)pImageData,
                                                       width, height, width * height * 4,
                                                       VK_FORMAT_R8G8B8A8_SRGB);
        material.colorTextureBlend = 1.0;

        writer.write_images(m_textureInputSet, 1, material.colorTexture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            {
                                    {
                                        .sampler = m_textureSampler,
                                        .imageView = m_colorTextures.view(material.colorTexture).view,
                                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                    }
                            });
    }

    if(materialData.normal_texture().has_value()) {
        int32_t width, height, numChannels;
        unsigned short* pImageData = load_image_data_16(pSceneData->textures()[materialData.normal_texture().value()].m_path,
                                                    &width, &height, &numChannels, 4);

        material.normalTexture = m_colorTextures.upload((unsigned char*)pImageData,
                                                        width, height, width * height * 8,
                                                        VK_FORMAT_R16G16B16A16_UNORM);

        writer.write_images(m_textureInputSet, 1, material.normalTexture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            {
                                    (VkDescriptorImageInfo){
                                            .sampler = m_textureSampler,
                                            .imageView = m_colorTextures.view(material.normalTexture).view,
                                            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                    }
                            });
        material.normalTextureBlend = 1.0;
    }

    if(materialData.metallic_roughness_texture().has_value()) {
        int32_t width, height, numChannels;
        unsigned char* pImageData = load_image_data(pSceneData->textures()[materialData.metallic_roughness_texture().value()].m_path,
                                                        &width, &height, &numChannels, 4);

        material.pbrTexture = m_colorTextures.upload((unsigned char*)pImageData,
                                                     width, height, width * height * 4,
                                                     VK_FORMAT_R8G8B8A8_UNORM);




        writer.write_images(m_textureInputSet, 1, material.pbrTexture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            {
                                    (VkDescriptorImageInfo){
                                            .sampler = m_textureSampler,
                                            .imageView = m_colorTextures.view(material.pbrTexture).view,
                                            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                    }
                            });
        material.pbrTextureBlend = 1.0;
    }

    char *pData;
    m_pGpu->memory()->map(m_materialBuffer.allocation, (void**)&pData);
    memcpy(pData + sizeof(Material) * distance, &material, sizeof(Material));
    m_pGpu->memory()->unmap(m_materialBuffer.allocation);

    m_materialsBits[distance] = true;

    writer.write();

    return distance;
}

std::vector<uint32_t> Scene::load_materials(const SceneData *pData) {
    std::vector<uint32_t> materials(pData->materials().size());
    uint32_t i = 0;
    for(auto& material : pData->materials()) {
        materials[i++] = push_material(pData, material);
    }

    return materials;
}


void Scene::pop_material(uint32_t materialIdx) {
    m_materialsBits[materialIdx] = false;
}

void Scene::add_scene_data(const SceneData *pData) {
    auto materialIds = load_materials(pData);

    m_meshBuffers.emplace_back(MeshBuffer(m_pGpu, &m_bufferWriter, pData->vertices(), pData->indices()),
                                         pData->primitives());

    m_meshBuffers[m_meshBuffers.size() - 1].remap_materials(materialIds);

    ShaderInputSetWriter writer(m_pGpu);
    writer.write_buffer(m_textureInputSet, 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {
            {
                .buffer = m_materialBuffer.buf,
                .offset = 0,
                .range = m_numMaterials * sizeof(Material)
            }
    }).write();
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