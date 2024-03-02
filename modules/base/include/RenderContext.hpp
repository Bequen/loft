#pragma once

#include <vulkan/vulkan_core.h>
#include "Gpu.hpp"
#include "RenderPassLayout.hpp"

class RenderContext {
private:
	VkCommandBuffer m_commandBuffer;
	Gpu *m_pGpu;
	RenderPassLayout *m_pLayout;
	VkFramebuffer m_framebuffer;

public:
	GET(m_commandBuffer, command_buffer);
	GET(m_framebuffer, framebuffer);
	GET(m_pGpu, gpu);
	GET(m_pLayout, layout);

	RenderContext(Gpu *pGpu, VkCommandBuffer commandBuffer,
				  RenderPassLayout *pLayout, VkFramebuffer framebuffer) :
		m_pGpu(pGpu), m_commandBuffer(commandBuffer), m_pLayout(pLayout),
		m_framebuffer(framebuffer) {
		
	}
};
