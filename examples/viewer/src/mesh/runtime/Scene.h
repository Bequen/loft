#pragma once

#include <utility>

#include "Gpu.hpp"
#include "Recording.hpp"
#include "mesh/data/SceneData.hpp"
#include "mesh/TextureStorage.h"
#include "mesh/MeshBuffer.h"
#include "shaders/ShaderInputSet.h"
#include "shaders/ShaderInputSetLayoutBuilder.hpp"
#include "Material.h"
#include "mesh/MaterialBuffer.h"
#include "mesh/Transform.hpp"
#include "mesh/TransformBuffer.hpp"

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

/**
 * Stores the rendered scene.
 */
class Scene {
private:
    const Gpu* m_gpu;

    std::vector<MeshScene> m_meshBuffers;

    /* material buffer */
    MaterialBuffer m_materialBuffer;

    lft::scene::TransformBuffer m_transform_buffer;

    Buffer m_lightBuffer;
    uint32_t m_numLights;

    ShaderInputSet m_textureInputSet;

    BufferBusWriter m_bufferWriter;
    ImageBusWriter m_textureWriter;

    VkSampler m_textureSampler;

    VkDescriptorSetLayout m_descriptorSetLayout;

    std::vector<uint32_t> load_materials(const SceneData* pData);

public:
    Scene(const Gpu* gpu, const SceneData *pData);

    uint32_t push_material(const SceneData *pData, const MaterialData& material);

    void pop_material(uint32_t materialIdx);

    void add_scene_data(const SceneData *pData);

    void draw(const lft::Recording& recording, const lft::RecordingBindPoint bind_point);

    void draw_depth(const lft::Recording& recording, const lft::RecordingBindPoint bind_point, mat4 transform);

    static ShaderInputSetLayoutBuilder input_layout() {
        return ShaderInputSetLayoutBuilder()
                /* material buffer */
                .uniform_buffer(0, VK_SHADER_STAGE_FRAGMENT_BIT)
                /* color texture */
                .n_images(1, 128, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
};
