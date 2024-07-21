#pragma once

#include <utility>

#include "Gpu.hpp"
#include "mesh/data/SceneData.hpp"
#include "mesh/TextureStorage.h"
#include "mesh/MeshBuffer.h"
#include "shaders/ShaderInputSet.h"
#include "shaders/ShaderInputSetLayoutBuilder.hpp"
#include "Material.h"
#include "mesh/MaterialBuffer.h"
#include "mesh/Transform.hpp"

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

    void remap_materials(std::vector<uint32_t> materialIds) {
        for(auto& primitive : m_primitives) {
            primitive.objectIdx = materialIds[primitive.objectIdx];
        }
    }
};

class Scene {
private:
    std::shared_ptr<const Gpu> m_gpu;

    std::vector<MeshScene> m_meshBuffers;

    /* material buffer */
    MaterialBuffer m_materialBuffer;

    /* transform buffer */
    Buffer m_transformBuffer;
    uint32_t m_numTransforms;
    std::vector<bool> m_transformBits;

    Buffer m_lightBuffer;
    uint32_t m_numLights;

    ShaderInputSet m_textureInputSet;

    BufferBusWriter m_bufferWriter;
    ImageBusWriter m_textureWriter;

    VkSampler m_textureSampler;

    VkDescriptorSetLayout m_descriptorSetLayout;

    std::vector<uint32_t> load_materials(const SceneData* pData);

public:
    Scene(const std::shared_ptr<const Gpu>& gpu, const SceneData *pData);

    uint32_t push_material(const SceneData *pData, const MaterialData& material);

    void pop_material(uint32_t materialIdx);

    void add_scene_data(const SceneData *pData);

    void draw(VkCommandBuffer cmdbuf, VkPipelineLayout layout);

    void draw_depth(VkCommandBuffer cmdbuf, VkPipelineLayout layout, mat4 transform);

    static ShaderInputSetLayoutBuilder input_layout() {
        return ShaderInputSetLayoutBuilder(2)
                /* material buffer */
                .binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                /* color texture */
                .binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
};
