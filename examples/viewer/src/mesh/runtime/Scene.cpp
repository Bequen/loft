//
// Created by martin on 3/31/24.
//

#include "Scene.h"
#include "SamplerBuilder.hpp"

#include <utility>

VkDescriptorSetLayout create_layout(const std::shared_ptr<const Gpu>& gpu) {
    return ShaderInputSetLayoutBuilder()
            /* material buffer */
            .binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                    /* color texture */
            .binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build(gpu);
}

Scene::Scene(std::shared_ptr<Gpu> gpu, const SceneData* pSceneData) :
    m_gpu(gpu),
    m_materialBuffer(gpu, pSceneData),
    m_transform_buffer(gpu, 100),
    m_bufferWriter(gpu, sizeof(Vertex) * 1024 * 256),
    m_textureWriter(gpu, nullptr, {1024, 1024}, 8, 8),
    m_descriptorSetLayout(create_layout(gpu)),
    m_textureInputSet(VK_NULL_HANDLE)
{
    m_textureInputSet = ShaderInputSet(gpu, m_descriptorSetLayout);

    const float min_lod = 0.0f;
    const float max_lod = (float)std::floor(std::log2(1024)) + 1;

    m_textureSampler = lft::SamplerBuilder()
        .filter(VK_FILTER_LINEAR)
        .mipmap_mode(VK_SAMPLER_MIPMAP_MODE_LINEAR)
        .address_mode(VK_SAMPLER_ADDRESS_MODE_REPEAT)
        .lod(min_lod, max_lod)
        .build(m_gpu);

    m_meshBuffers.emplace_back(MeshBuffer(m_gpu, &m_bufferWriter, 
                               pSceneData->vertices(), pSceneData->indices()),
                               pSceneData->primitives());

    std::vector<VkDescriptorImageInfo> imageWriters(/* pSceneData->num_textures() + 1 */128);
    for(int i = 0; i < 128; i++) {
        imageWriters[i] = {
                .sampler = m_textureSampler,
                .imageView = m_materialBuffer.m_textureStorage.view(i % pSceneData->num_textures()).view,
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

void Scene::draw(const lft::Recording& recording, const lft::RecordingBindPoint bind_point) {
    auto descriptorSet = m_textureInputSet.descriptor_set();
    bind_point.bind_descriptor_set(2, descriptorSet);

    for(auto& meshBuffer : m_meshBuffers) {
        size_t offsets[] = {0};
        recording.bind_vertex_buffers({meshBuffer.vertex_buffer()}, {0});
        recording.bind_index_buffer(meshBuffer.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

        for(auto& primitive : meshBuffer.primitives()) {
            uint32_t constants[2] = {0, primitive.objectIdx};
            bind_point.push_constants(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0, sizeof(uint32_t) * 2, constants);
            recording.draw_indexed(primitive.count, 1, primitive.offset, primitive.baseVertex, 0);
        }
    }
}

void Scene::draw_depth(const lft::Recording& recording, const lft::RecordingBindPoint bind_point,
        mat4 transform) {
    for(auto& meshBuffer : m_meshBuffers) {
        size_t offsets[] = {0};

        recording.bind_vertex_buffers({meshBuffer.vertex_buffer()}, {0});
        recording.bind_index_buffer(meshBuffer.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

        for(auto& primitive : meshBuffer.primitives()) {
            bind_point.push_constants(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0, sizeof(mat4), transform);
            recording.draw_indexed(primitive.count, 1, primitive.offset, primitive.baseVertex, 0);
        }
    }
}
