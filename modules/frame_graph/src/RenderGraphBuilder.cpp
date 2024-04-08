#include "RenderGraphBuilder.h"
#include "RenderGraph.hpp"

#include "DebugUtils.h"

#include <queue>
#include <stack>
#include <utility>


RenderGraphBuilder::RenderGraphBuilder(
        std::string name,
        std::string outputName,
        uint32_t numImageInFlight) :

        m_name(std::move(name)),
        m_outputName(std::move(outputName)),
        m_numImagesInFlight(numImageInFlight),
        m_numRenderpasses(0) {
}

VkExtent2D RenderGraphBuilder::extent_for(RenderPass *pPass) const {
    return {
            pPass->extent().width == 0 ? m_extent.width : pPass->extent().width,
            pPass->extent().height == 0 ? m_extent.height : pPass->extent().height
    };
}

VkRenderPass RenderGraphBuilder::create_renderpass_for(Gpu *pGpu, RenderGraphNode *pPass) {
    // Create storage for all the attachments
    auto depthOutput = pPass->renderpass()->depth_output();
    std::vector<VkAttachmentDescription2> attachments(pPass->renderpass()->num_outputs() + depthOutput.has_value());
    std::vector<VkAttachmentReference2> references(pPass->renderpass()->num_outputs() + depthOutput.has_value());

    uint i = 0;

    for(auto output : pPass->renderpass()->outputs()) {
        auto layout = output->name() == "swapchain" ?
                      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :
                      (VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        attachments[i] = (VkAttachmentDescription2){
                .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
                .format = ((ImageResourceLayout*)output)->format(),
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = pPass->renderpass()->pass_dependencies().empty() ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = layout,
        };

        references[i] = (VkAttachmentReference2) {
                .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                .attachment = i,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
        i++;
    }

    if(depthOutput.has_value()) {
        auto layout = depthOutput.value()->name() == "swapchain" ?
                      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :
                      (VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // add depth attachment and reference
        attachments[i] = (VkAttachmentDescription2){
                .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
                .format = depthOutput.value()->format(),
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = pPass->renderpass()->pass_dependencies().empty() ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = layout,
                .finalLayout = layout,
        };

        references[i] = (VkAttachmentReference2) {
                .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                .attachment = i,
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        i++;
    }

    VkMemoryBarrier2KHR entryBarrier = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT_KHR
    };

    VkMemoryBarrier2KHR exitBarrier = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT_KHR
    };

    const VkSubpassDependency2 subpassDependencies[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
                    .pNext = &entryBarrier,
                    .srcSubpass = VK_SUBPASS_EXTERNAL,
                    .dstSubpass = 0,
                    .dependencyFlags = 0,
            }, {
                    .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
                    .pNext = &exitBarrier,
                    .srcSubpass = 0,
                    .dstSubpass = VK_SUBPASS_EXTERNAL,
                    .dependencyFlags = 0
            }
    };

    const VkSubpassDescription2 subpass = {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = (uint32_t)references.size() - (depthOutput.has_value()),
            .pColorAttachments = references.data(),
            .pDepthStencilAttachment = depthOutput.has_value() ? &references[i - 1] : nullptr,
    };

    const VkRenderPassCreateInfo2 renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,

            .attachmentCount = (uint32_t)attachments.size(),
            .pAttachments = attachments.data(),

            .subpassCount = 1,
            .pSubpasses = &subpass,

            .dependencyCount = 1,
            .pDependencies = subpassDependencies
    };

    VkRenderPass renderpass;
    if(vkCreateRenderPass2(pGpu->dev(), &renderPassInfo, nullptr, &renderpass)) {
        throw std::runtime_error("Failed to create renderpass");
    }

    VkDebugUtilsObjectNameInfoEXT nameInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType                    = VK_OBJECT_TYPE_RENDER_PASS,
            .objectHandle                  = (uint64_t)renderpass,
            .pObjectName                   = pPass->renderpass()->name().c_str(),
    };

    vkSetDebugUtilsObjectName(pGpu->dev(), &nameInfo);

    return renderpass;
}

Image RenderGraphBuilder::create_image_from_layout(Gpu *pGpu, VkExtent2D extent, ImageResourceLayout *pLayout, bool isDepth) {
    MemoryAllocationInfo memoryInfo = {
            .usage = MEMORY_USAGE_AUTO_PREFER_DEVICE
    };

    /* create image resource */
    ImageCreateInfo imageInfo = {
            .extent = extent,
            .format = pLayout->format(),
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                     (VkImageUsageFlags)(isDepth ?
                                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :
                                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
            .aspectMask = isDepth ?
                          VK_IMAGE_ASPECT_DEPTH_BIT :
                          VK_IMAGE_ASPECT_COLOR_BIT,
            .arrayLayers = 1,
            .mipLevels = 1,
    };

    Image image = {};
    pGpu->memory()->create_image(&imageInfo, &memoryInfo, &image);

    return image;
}

std::vector<ImageView>
RenderGraphBuilder::get_attachment(Gpu *pGpu, RenderGraphBuilderCache *pCache,
                                   uint32_t numImagesInFlight, VkExtent2D extent,
                                   bool isDepth, ImageResourceLayout* pLayout) {
    std::vector<Image> images(numImagesInFlight);
    std::vector<ImageView> views(numImagesInFlight);

    auto aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    /* create image for each frame */
    for(uint32_t f = 0; f < numImagesInFlight; f++) {
        images[f] = create_image_from_layout(pGpu, extent, pLayout, isDepth);
        images[f].set_debug_name(pGpu, pLayout->name());

        views[f] = images[f].create_view(pGpu, pLayout->format(),
                                         (VkImageSubresourceRange) {
                                                 .aspectMask = aspectMask,
                                                 .baseMipLevel = 0,
                                                 .levelCount = 1,
                                                 .baseArrayLayer = 0,
                                                 .layerCount = 1,
                                         });

        views[f].set_debug_name(pGpu, pLayout->name() + "_view");
    }

    pCache->cache_image(pLayout->name(), images, views, isDepth);

    return views;
}



std::vector<Framebuffer> RenderGraphBuilder::collect_attachments(Gpu *pGpu, VkRenderPass rp, RenderGraphBuilderCache *pCache, RenderGraphNode *pPass) {
    uint32_t numOutputs = pPass->renderpass()->num_outputs() + pPass->renderpass()->depth_output().has_value();

    std::vector<std::vector<ImageView>> attachments(pCache->output_chain().count(),
                                                    std::vector<ImageView>(numOutputs));
    std::vector<VkClearValue> clearValues(numOutputs);

    auto chainOutput = pPass->renderpass()->output(pCache->output_name());
    auto extent = extent_for(pPass->renderpass());

    if(chainOutput.has_value()) {
        std::vector<Framebuffer> framebuffers(pCache->output_chain().count(), VK_NULL_HANDLE);

        for(uint32_t f = 0; f < pCache->output_chain().count(); f++) {
            framebuffers[f] = FramebufferBuilder(rp, extent, { pCache->output_chain().views()[f] }).build(pGpu);
        }

        clearValues[0] = ((ImageResourceLayout*)chainOutput.value())->clear_value();
        pPass->set_clear_values(clearValues);

        return framebuffers;
    }

    uint32_t i = 0;
    auto depthOutput = pPass->renderpass()->depth_output();

    for(auto attachment : pPass->renderpass()->outputs()) {
        if(attachment->resource_type() != RESOURCE_TYPE_IMAGE) continue;

        auto views = get_attachment(pGpu, pCache, m_numImagesInFlight, extent,
                                    false, (ImageResourceLayout*)attachment);

        for(uint32_t o = 0; o < m_numImagesInFlight; o++) {
            attachments[o][i] = views[o];
        }

        clearValues[i++] = (((ImageResourceLayout*)attachment)->clear_value());
    }

    if(depthOutput.has_value()) {
        auto views = get_attachment(pGpu, pCache, m_numImagesInFlight, extent_for(pPass->renderpass()),
                                    true, (ImageResourceLayout*)depthOutput.value());

        for(uint32_t o = 0; o < m_numImagesInFlight; o++) {
            attachments[o][i] = views[o];
        }

        clearValues[i] = (depthOutput.value()->clear_value());

        i++;
    }

    pPass->set_clear_values(clearValues);

    std::vector<Framebuffer> framebuffers(m_numImagesInFlight, VK_NULL_HANDLE);

    for(uint32_t f = 0; f < m_numImagesInFlight; f++) {
        framebuffers[f] = FramebufferBuilder(rp,
                                             extent_for(pPass->renderpass()),
                                             attachments[f])
                                .build(pGpu);
    }

    return framebuffers;
}

void RenderGraphBuilder::build_renderpass_dependencies(Gpu *pGpu, RenderGraphBuilderCache *pCache,
                                                       RenderGraphVkCommandBuffer *pCmdBuf, uint32_t renderpassIdx,
                                                       int depth) {
    for(uint32_t i = 0; i < m_numRenderpasses; i++) {
        if(pCache->adjacency_matrix().get(i, renderpassIdx)) {
            RenderGraphVkCommandBuffer *pNextCmdBuf = pCmdBuf;
            if(pCache->adjacency_matrix().num_dependencies(renderpassIdx) > 1) {
                pNextCmdBuf = new RenderGraphVkCommandBuffer(pGpu, m_numImagesInFlight);
                pCmdBuf->add_dependency(pNextCmdBuf);
            }

            depth = build_renderpass(pGpu, pCache, pNextCmdBuf, &m_renderpasses[i], depth, i);
        }
    }
}

std::vector<uint32_t>
RenderGraphBuilder::get_external_dependency_ids_for_renderpass(RenderGraphBuilderCache *pCache,
                                                               RenderGraphVkCommandBuffer *pCmdBuf,
                                                               uint32_t renderpassIdx) {
    std::vector<uint32_t> externalDependencies = pCmdBuf->m_externalDependenciesIds;
    for(uint32_t i = m_numRenderpasses; i < m_numRenderpasses + m_externalImageDependencies.size(); i++) {
        if(pCache->adjacency_matrix().get(i, renderpassIdx)) {
            externalDependencies.push_back(i - m_numRenderpasses);
        }
    }

    return externalDependencies;
}

int RenderGraphBuilder::build_renderpass(Gpu *pGpu, RenderGraphBuilderCache *pCache,
                                          RenderGraphVkCommandBuffer *pCmdBuf, RenderGraphNode *pPass,
                                          int depth, uint32_t renderpassIdx) {
    if(pPass->is_visited()) return 0;

    build_renderpass_dependencies(pGpu, pCache, pCmdBuf, renderpassIdx, depth - 1);

    pCmdBuf->set_external_dependencies(get_external_dependency_ids_for_renderpass(pCache, pCmdBuf, renderpassIdx));

    /* does not matter, how many framebuffers returns */
    auto rp = create_renderpass_for(pGpu, pPass);
    auto framebuffers = collect_attachments(pGpu, rp, pCache, pPass);
    auto extent = extent_for(pPass->renderpass());

    pCmdBuf->add_render_pass({
                                 pPass->renderpass()->name(),
                                 pPass->renderpass(),
                                 rp,
                                 extent,
                                 pPass->clear_values(),
                                 framebuffers,
                                 pPass->renderpass()->output(m_outputName).has_value()
                            });

    // wrap it into single structure
    auto outputInfo = RenderPassOutputInfo(rp, framebuffers, extent);
    auto buildInfo = RenderPassBuildInfo(pGpu, pCache, outputInfo, pCache->sampler(), m_numImagesInFlight);

    pPass->renderpass()->prepare(buildInfo);

    pPass->mark_visited();

    return 1;
}

std::vector<RenderGraphVkCommandBuffer>
RenderGraphBuilder::build_renderpasses(Gpu *pGpu, RenderGraphBuilderCache *pCache) {
    std::vector<uint32_t> dependencies = pCache->adjacency_matrix().get_dependencies(m_numRenderpasses + m_externalImageDependencies.size());

    /* create command buffer for each dependency */
    std::vector<RenderGraphVkCommandBuffer> cmdbufs(dependencies.size(), RenderGraphVkCommandBuffer(pGpu, m_numImagesInFlight));

    uint32_t i = 0;
    for(auto dependency : dependencies) {
        build_renderpass(pGpu, pCache, &cmdbufs[i++], &m_renderpasses[dependency], 0, dependency);
    }

    return cmdbufs;
}

RenderGraph RenderGraphBuilder::build(Gpu *pGpu, const ImageChain& outputChain, VkExtent2D extent) {
    m_extent = extent;

    // Build matrix
    auto adjacencyMatrix = build_adjacency_matrix();
    adjacencyMatrix.transitive_reduction();

    auto sampler = create_attachment_sampler(pGpu);

    // Prepare cache
    RenderGraphBuilderCache cache(m_outputName, m_numImagesInFlight, adjacencyMatrix, sampler, outputChain);

    auto queue = build_renderpasses(pGpu, &cache);

    cache.transfer_layout(pGpu);

    auto graph = RenderGraph(pGpu, m_name, outputChain, queue, m_numImagesInFlight);

    for(const auto& externalDependency : m_externalImageDependencies) {
        graph.set_external_dependency(externalDependency.name, VK_NULL_HANDLE);
    }

    return graph;
}

const AdjacencyMatrix
RenderGraphBuilder::build_adjacency_matrix() const {
    AdjacencyMatrix matrix(m_numRenderpasses + m_externalImageDependencies.size() + 1);

    for(uint32_t x = 0; x < m_numRenderpasses; x++) {
        for(uint32_t y = 0; y < m_numRenderpasses; y++) {
            if(x == y) continue;

            if(m_renderpasses[x].renderpass()->has_pass_dependency(m_renderpasses[y].renderpass()->name())) {
                matrix.set(y, x);
                continue;
            }

            for(auto& output : m_renderpasses[x].renderpass()->outputs()) {
                if(std::count(m_renderpasses[y].renderpass()->inputs().begin(),
                              m_renderpasses[y].renderpass()->inputs().end(),
                              output->name()) > 0) {
                    matrix.set(x, y);
                    break;
                }
            }
        }

        for(uint32_t i = 0; i < m_externalImageDependencies.size(); i++) {
            if(std::count(m_renderpasses[x].renderpass()->inputs().begin(),
                          m_renderpasses[x].renderpass()->inputs().end(),
                          m_externalImageDependencies[i].name) > 0) {
                matrix.set(m_numRenderpasses + i, x);
            }
        }

        if(m_renderpasses[x].renderpass()->output(m_outputName).has_value()) {
            matrix.set(x, m_numRenderpasses + m_externalImageDependencies.size());
        }
    }

    return matrix;
}

VkSampler RenderGraphBuilder::create_attachment_sampler(Gpu *pGpu) {
    VkSamplerCreateInfo samplerInfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE
    };

    VkSampler sampler = VK_NULL_HANDLE;
    if(vkCreateSampler(pGpu->dev(), &samplerInfo, nullptr, &sampler)) {
        throw std::runtime_error("failed to create sampler");
    }

    return sampler;
}