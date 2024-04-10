#pragma once

#include "SceneData.hpp"
#include "MaterialBuffer.h"
#include "resources/Buffer.hpp"
#include "Drawable.hpp"
#include "Gpu.hpp"
#include "DrawMeshInfo.hpp"
#include "shaders/ShaderInputSet.h"
#include <vulkan/vulkan_core.h>

/**
 * SceneBuffer
 * Drawable of entire scene. For example glTF scene.
 * Stores information about the GPU buffers only.
 */
class SceneBuffer {
public:
	Buffer m_vertexBuffer;
	Buffer m_indexBuffer;

	Buffer m_transformBuffer;
    Buffer m_objectInfoBuffer;

	MaterialBuffer m_materialBuffer;

    std::vector<Primitive> m_primitives;

    uint32_t m_numTransparentPrimitives;

public:
    /**
     * Creates new scene buffer from scene data
     * @param pGpu Gpu used to allocate resources etc.
     * @param pData Data to be used to create scene buffer
     * @return Returns new scene buffer created from scene data
     */
    SceneBuffer(Gpu *pGpu, SceneData *pData);

    void draw_opaque(VkCommandBuffer cmdbuf, VkPipelineLayout layout);
    void draw_depth(VkCommandBuffer cmdbuf, VkPipelineLayout layout);
};
