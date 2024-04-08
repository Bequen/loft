#pragma once

#include <utility>

#include "Gpu.hpp"
#include "../mesh/SceneData.hpp"
#include "../mesh/TextureStorage.h"
#include "../mesh/MeshBuffer.h"
#include "shaders/ShaderInputSet.h"
#include "shaders/ShaderInputSetLayoutBuilder.hpp"
#include "Material.h"

struct SceneObject {
    uint32_t m_transformIdx;
    uint32_t m_materialIdx;
};

class MeshScene {
    MeshBuffer m_buffer;
    std::vector<Primitive> m_primitives;

public:
    inline const std::vector<Primitive>& primitives() const {
        return m_primitives;
    }

    inline const Buffer& vertex_buffer() const {
        return m_buffer.vertex_buffer();
    }
    inline const Buffer& index_buffer() const {
        return m_buffer.index_buffer();
    }

    MeshScene(MeshScene&& meshScene)  noexcept :
            m_buffer(std::move(meshScene.m_buffer)),
            m_primitives(std::move(meshScene.m_primitives)) {

    }

    MeshScene(MeshBuffer buffer, const std::vector<Primitive>& primitives) :
            m_buffer(std::move(buffer)), m_primitives(std::move(primitives)) {

    }
};

class Scene {
private:
    Gpu *m_pGpu;

    std::vector<MeshScene> m_meshBuffers;

    /* material buffer */
    Buffer m_materialBuffer;
    uint32_t m_numMaterials;
    std::vector<bool> m_materialsBits;

    /* transform buffer */
    Buffer m_transformBuffer;
    uint32_t m_numTransforms;
    std::vector<bool> m_transformBits;

    Buffer m_lightBuffer;
    uint32_t m_numLights;

    /* texture storages */
    TextureStorage m_colorTextures;
    TextureStorage m_normalTextures;
    TextureStorage m_pbrTextures;

    BufferBusWriter m_bufferWriter;
    ImageBusWriter m_textureWriter;

    VkSampler m_textureSampler;

    VkDescriptorSet m_descriptorSet;

    std::vector<uint32_t> load_materials(const SceneData* pData);

public:
    Scene(Gpu *pGpu);

    uint32_t push_material(const SceneData *pData, const MaterialData& material);

    void pop_material(uint32_t materialIdx);

    void add_scene_data(const SceneData *pData);

    void draw(VkCommandBuffer cmdbuf, VkPipelineLayout layout);

    ShaderInputSetBuilder input_set() {
        return ShaderInputSetBuilder(4)
                /* material buffer */
                .buffer(0, m_materialBuffer, 0, m_numMaterials * sizeof(Material))
                /* color texture storage */
                .image(1, m_colorTextures.view(), m_textureSampler)
                /* normal texture storage */
                .image(2, m_normalTextures.view(), m_textureSampler)
                /* pbr texture storage */
                .image(3, m_pbrTextures.view(), m_textureSampler);
    }

    static ShaderInputSetLayoutBuilder input_layout() {
        return ShaderInputSetLayoutBuilder(4)
                /* material buffer */
                .binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                        /* color texture */
                .binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                        /* normal texture */
                .binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                        /* pbr texture */
                .binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
};
