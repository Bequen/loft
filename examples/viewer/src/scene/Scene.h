#pragma once

#include "Gpu.hpp"
#include "../mesh/SceneData.hpp"
#include "../mesh/TextureStorage.h"
#include "../mesh/MeshBuffer.h"

class Scene {
private:
    Gpu *m_pGpu;

    std::vector<MeshBuffer> m_meshBuffers;

    /* material buffer */
    Buffer m_materialBuffer;
    uint32_t m_numMaterials;
    std::vector<bool> m_materialsBits;

    /* transform buffer */
    Buffer m_transformBuffer;
    uint32_t m_numTransforms;
    std::vector<bool> m_transformBits;

    /* texture storages */
    TextureStorage m_colorTextures;
    TextureStorage m_normalTextures;
    TextureStorage m_pbrTextures;

    BufferBusWriter m_bufferWriter;
    ImageBusWriter m_textureWriter;

public:
    Scene(Gpu *pGpu);

    void add_scene_data(SceneData *pData);
};
