//
// Created by martin on 3/31/24.
//

#include "Scene.h"
#include "../mesh/MaterialBuffer.h"

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

    materialsBufferInfo.size = sizeof(Transform) * 256;
    pGpu->memory()->create_buffer(&materialsBufferInfo, &memoryAllocationInfo,
                                  &m_transformBuffer);
}

void Scene::add_scene_data(SceneData *pData) {
    m_meshBuffers.emplace_back({m_pGpu, &m_bufferWriter, pData->vertices(), pData->indices()});
}