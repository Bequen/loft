#include "SceneBuffer.hpp"

#include "resources/BufferBusWriter.h"
#include "RenderContext.hpp"
#include "shaders/ShaderInputSet.h"
#include <vulkan/vulkan_core.h>
#include <string.h>


SceneBuffer::SceneBuffer(Gpu *pGpu, SceneData *pData) :
        m_materialBuffer(pGpu, {2048, 2048}, pData) {

    MemoryAllocationInfo memoryAllocationInfo = {
            .usage = MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };



	/**
	 * Create & upload vertex buffer
	 */
	BufferCreateInfo vertexBufferInfo = {
		.size = pData->num_vertices() * sizeof(Vertex),
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		.isExclusive = true,
	};
	pGpu->memory()->create_buffer(&vertexBufferInfo, &memoryAllocationInfo,
								  &m_vertexBuffer);

    auto bus = BufferBusWriter(pGpu, &m_vertexBuffer, vertexBufferInfo.size);
	bus.write((void*)pData->vertices().data(), 0, vertexBufferInfo.size);
    bus.flush();

	/**
	 * Create & upload index buffer
	 */
	BufferCreateInfo indexBufferInfo = {
		.size = pData->num_indices() * sizeof(Index),
		.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		.isExclusive = true,
	};
	pGpu->memory()->create_buffer(&indexBufferInfo, &memoryAllocationInfo,
								  &m_indexBuffer);


	bus.set_buffer(&m_indexBuffer)
		.write((void*)pData->indices().data(), 0, indexBufferInfo.size);
    bus.flush();
    bus.wait();

	m_primitives = std::vector<Primitive>(pData->primitives().size());

    m_numTransparentPrimitives = 0;
    uint32_t maxPrimitives = pData->primitives().size();

    /* sort transparent objects to the back */
    for(int i = 0; i < pData->primitives().size(); i++) {
        if(pData->materials()[pData->primitives()[i].materialIdx].is_blended()) {
            m_primitives[maxPrimitives - m_numTransparentPrimitives - 1] = pData->primitives()[i];
            m_numTransparentPrimitives++;
        } else {
            m_primitives[i - m_numTransparentPrimitives] = pData->primitives()[i];
        }
    }
}

void SceneBuffer::draw_opaque(VkCommandBuffer cmdbuf, VkPipelineLayout layout) {
    size_t offsets[] = {0};
    vkCmdBindVertexBuffers(cmdbuf, 0, 1, &m_vertexBuffer.buf, offsets);
    vkCmdBindIndexBuffer(cmdbuf, m_indexBuffer.buf, 0, VK_INDEX_TYPE_UINT32);

    for(uint32_t i = 0; i < m_primitives.size() - m_numTransparentPrimitives; i++) {
        const auto pPrimitive = &m_primitives[i];
        uint32_t values[2] = {0, pPrimitive->materialIdx};
        vkCmdPushConstants(cmdbuf, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) * 2, values);
        vkCmdDrawIndexed(cmdbuf, pPrimitive->count, 1, pPrimitive->offset, pPrimitive->baseVertex, 0);
    }
}

void SceneBuffer::draw_depth(VkCommandBuffer cmdbuf, VkPipelineLayout layout) {
    size_t offsets[] = {0};
    vkCmdBindVertexBuffers(cmdbuf, 0, 1, &m_vertexBuffer.buf, offsets);
    vkCmdBindIndexBuffer(cmdbuf, m_indexBuffer.buf, 0, VK_INDEX_TYPE_UINT32);

    for(uint32_t i = 0; i < m_primitives.size() - m_numTransparentPrimitives; i++) {
        const auto pPrimitive = &m_primitives[i];
        vkCmdDrawIndexed(cmdbuf, pPrimitive->count, 1, pPrimitive->offset, pPrimitive->baseVertex, 0);
    }
}
