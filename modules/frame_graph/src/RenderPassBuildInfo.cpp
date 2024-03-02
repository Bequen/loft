#include "RenderPassBuildInfo.hpp"

#include "RenderPass.hpp"
#include "ResourceType.h"
#include "shaders/ShaderInputSet.h"

#if 0

/* std::vector<ImageResource*> 
RenderPassBuildInfo::collect_attachments(RenderPass *pRenderPass) {
	std::vector<ImageResource*> attachments(pRenderPass->num_outputs());
	uint32_t i = 0;
	for(auto& resource : pRenderPass->outputs()) {
		if(resource->resource_type() == RESOURCE_TYPE_IMAGE) {
			attachments[i++] = (ImageResource*)(*m_pTargets)[resource->name()];
		}
	}

	return attachments;
} */


/* std::vector<std::vector<ShaderInputSetWrite>>
RenderPassBuildInfo::collect_inputs(RenderPass *pRenderPass) {
    std::vector<std::vector<ShaderInputSetWrite>> inputs(pRenderPass->num_frames(),
                                                         std::vector<ShaderInputSetWrite>(pRenderPass->num_inputs()));

    uint32_t i = 0;
    for(auto& resource : pRenderPass->inputs()) {
        for(uint32_t f = 0; f < pRenderPass->num_frames(); f++) {
            
        }
        auto target = m_pTargets->at(resource);
    }

    return inputs;
} */
#endif
