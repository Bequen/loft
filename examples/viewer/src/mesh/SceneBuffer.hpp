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
class SceneBuffer : Drawable  {
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

	/**
	 * Draws the scene by using RenderContext
	 *
	 * @param pContext rendering context to use. Cannot be null.
	 */
	void draw(RenderContext *pContext) override;

    void draw_opaque(VkCommandBuffer cmdbuf, VkPipelineLayout layout);
    void draw_opaque(RenderContext *pContext);
    void draw_transparent(RenderContext *pContext);

    void set_writes(ShaderInputSetBuilder &builder);

	std::vector<VkDescriptorSetLayoutBinding> bindings() {
		VkDescriptorSetLayoutBinding predefined = {
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL
		};
		auto bindings = std::vector<VkDescriptorSetLayoutBinding>(5, predefined);

		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		bindings[1].binding = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;


		bindings[2].binding = 2;
		bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

		bindings[3].binding = 3;
		bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

		bindings[4].binding = 4;
		bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

		return bindings;
	}
};
