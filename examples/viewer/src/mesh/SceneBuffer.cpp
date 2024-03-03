#include "SceneBuffer.hpp"

#include "resources/BufferBusWriter.h"
#include "RenderContext.hpp"
#include "shaders/ShaderInputSet.h"
#include <vulkan/vulkan_core.h>
#include <string.h>


SceneBuffer::SceneBuffer(Gpu *pGpu, SceneData *pData) :
        m_materialBuffer(pGpu, {1024, 1024}, pData) {

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

    printf("Vertex buffer start\n");
    auto bus = BufferBusWriter(pGpu, &m_vertexBuffer, vertexBufferInfo.size);
	bus.write((void*)pData->vertices().data(), 0, vertexBufferInfo.size);
    bus.flush();
    printf("Vertex buffer done\n");

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

    FILE *file = fopen("/home/martin/indices2.csv", "w");
    for(uint32_t i = 0; i < pData->num_indices(); i++) {
        auto vertex = ((Index*)pData->indices().data())[i];
        fprintf(file, "%u\n", vertex);
    }
    fclose(file);

	bus.set_buffer(&m_indexBuffer)
		.write((void*)pData->indices().data(), 0, indexBufferInfo.size);
    bus.flush();
    printf("Index buffer done\n");



	/**
	 * Create & upload object info buffer (model matrices)
	 */
    /* BufferCreateInfo objectInfoBuffer = {
            .size = pData->num_nodes() * sizeof(SceneNode),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .isExclusive = true,
    };
    pGpu->memory()->create_buffer(&objectInfoBuffer, &memoryAllocationInfo,
								  &m_objectInfoBuffer);

	bus.set_buffer(&m_objectInfoBuffer)
		.write((void*)pData->nodes().data(), 0, objectInfoBuffer.size); */
 
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

void SceneBuffer::draw_opaque(RenderContext *pContext) {
    size_t offsets[] = {0};
    vkCmdBindVertexBuffers(pContext->command_buffer(), 0, 1, &m_vertexBuffer.buf, offsets);
    vkCmdBindIndexBuffer(pContext->command_buffer(), m_indexBuffer.buf, 0, VK_INDEX_TYPE_UINT32);

    for(uint32_t i = 0; i < m_primitives.size() - m_numTransparentPrimitives; i++) {
        const auto pPrimitive = &m_primitives[i];
        uint32_t values[2] = {0, pPrimitive->materialIdx};
        vkCmdPushConstants(pContext->command_buffer(), pContext->layout()->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) * 2, values);
        vkCmdDrawIndexed(pContext->command_buffer(), pPrimitive->count, 1, pPrimitive->offset, pPrimitive->baseVertex, 0);
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

void SceneBuffer::draw_transparent(RenderContext *pContext) {
    size_t offsets[] = {0};
    vkCmdBindVertexBuffers(pContext->command_buffer(), 0, 1, &m_vertexBuffer.buf, offsets);
    vkCmdBindIndexBuffer(pContext->command_buffer(), m_indexBuffer.buf, 0, VK_INDEX_TYPE_UINT32);

    for(uint32_t i = m_primitives.size() - m_numTransparentPrimitives; i < m_primitives.size(); i++) {
        const auto pPrimitive = &m_primitives[i];
        uint32_t values[2] = {0, pPrimitive->materialIdx};
        vkCmdPushConstants(pContext->command_buffer(), pContext->layout()->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) * 2, values);
        vkCmdDrawIndexed(pContext->command_buffer(), pPrimitive->count, 1, pPrimitive->offset, pPrimitive->baseVertex, 0);
    }
}

void SceneBuffer::draw(RenderContext *pContext) {
    draw_opaque(pContext);
    draw_transparent(pContext);
}

void SceneBuffer::set_writes(ShaderInputSetBuilder& builder) {
    /* builder.set_write(0, std::make_shared<ShaderInputSetBufferWrite>(0, m_materialBuffer.material_buffer()));
    builder.set_write(1, std::make_shared<ShaderInputSetBufferWrite>(1, m_objectInfoBuffer));

    builder.set_write(2, std::make_shared<ShaderInputSetImageWrite>(2, m_materialBuffer.color_texture(), m_materialBuffer.sampler()));
    builder.set_write(3, std::make_shared<ShaderInputSetImageWrite>(3, m_materialBuffer.normal_texture(), m_materialBuffer.sampler()));
    builder.set_write(4, std::make_shared<ShaderInputSetImageWrite>(4, m_materialBuffer.pbr_texture(), m_materialBuffer.sampler())); */
}
