#include "RenderGraphBuilder.h"
#include "RenderGraph.hpp"

#include "DebugUtils.h"

#include <queue>
#include <stack>


RenderGraphBuilder::RenderGraphBuilder(uint32_t numImageInFlight, uint32_t numOutputImages, VkExtent2D extent) :
        m_extent(extent), m_numFrames(numImageInFlight), m_numChainImages(numOutputImages), m_numRenderpasses(0) {
}

VkExtent2D RenderGraphBuilder::extent_for(RenderPass *pPass) const {
    return {
            pPass->extent().width == 0 ? m_extent.width : pPass->extent().width,
            pPass->extent().height == 0 ? m_extent.height : pPass->extent().height
    };
}

VkRenderPass create_renderpass_for(Gpu *pGpu, RenderGraphNode *pPass) {
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
                .initialLayout = layout,
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
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT_KHR
    };

    VkMemoryBarrier2KHR exitBarrier = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR,
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

            .dependencyCount = 2,
            .pDependencies = subpassDependencies
    };

    VkRenderPass renderpass;
    if(vkCreateRenderPass2(pGpu->dev(), &renderPassInfo, nullptr, &renderpass)) {
        throw std::runtime_error("Failed to create renderpass");
    }

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
        VkDebugUtilsObjectNameInfoEXT nameInfo = {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                .objectType                    = VK_OBJECT_TYPE_IMAGE,
                .objectHandle                  = (uint64_t)images[f].img,
                .pObjectName                   = pLayout->name().c_str(),
        };

        vkSetDebugUtilsObjectName(pGpu->dev(), &nameInfo);

        views[f] = images[f].create_view(pGpu, pLayout->format(),
                                         (VkImageSubresourceRange) {
                                                 .aspectMask = aspectMask,
                                                 .baseMipLevel = 0,
                                                 .levelCount = 1,
                                                 .baseArrayLayer = 0,
                                                 .layerCount = 1,
                                         });

        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        nameInfo.objectHandle = (uint64_t)views[f].view;
        nameInfo.pObjectName = (pLayout->name() + "_view").c_str();
        vkSetDebugUtilsObjectName(pGpu->dev(), &nameInfo);
    }

    pCache->cache_image(pLayout->name(), images, views, isDepth);

    return views;
}

std::vector<Framebuffer> RenderGraphBuilder::collect_attachments(Gpu *pGpu, VkRenderPass rp, RenderGraphBuilderCache *pCache, RenderGraphNode *pPass) {
    std::vector<std::vector<ImageView>> attachments(num_frames(), std::vector<ImageView>(pPass->renderpass()->num_outputs() + pPass->renderpass()->depth_output().has_value()));
    std::vector<VkClearValue> clearValues(pPass->renderpass()->num_outputs() + pPass->renderpass()->depth_output().has_value());

    // Create swapchain framebuffers
    if(pPass->renderpass()->outputs().size() == 1 &&
       pPass->renderpass()->outputs()[0]->name() == pCache->output_name()) {
        std::vector<Framebuffer> framebuffers(pCache->output_chain().count(), VK_NULL_HANDLE);

        for(uint32_t f = 0; f < pCache->output_chain().count(); f++) {
            framebuffers[f] = FramebufferBuilder(rp, extent_for(pPass->renderpass()),
                                                 { pCache->output_chain().views()[f] })
                    .build(pGpu);
        }

        clearValues[0] = (VkClearValue {
                .color = {0.0f, 0.0f, 0.0f, 0.0f}
        });

        return framebuffers;
    }

    uint32_t i = 0;
    auto depthOutput = pPass->renderpass()->depth_output();

    for(auto attachment : pPass->renderpass()->outputs()) {
        if(attachment->resource_type() != RESOURCE_TYPE_IMAGE) continue;
        if(attachment->name() == pCache->output_name()) {
            throw std::runtime_error("Cannot use swapchain output next to other outputs");
        }

        auto views = get_attachment(pGpu, pCache, num_frames(), extent_for(pPass->renderpass()),
                                    false, (ImageResourceLayout*)attachment);

        for(uint32_t o = 0; o < num_frames(); o++) {
            attachments[o][i] = views[o];
        }

        clearValues[i] = (((ImageResourceLayout*)attachment)->clear_value());

        i++;
    }

    if(depthOutput.has_value()) {
        auto views = get_attachment(pGpu, pCache, num_frames(), extent_for(pPass->renderpass()),
                                    true, (ImageResourceLayout*)depthOutput.value());

        for(uint32_t o = 0; o < num_frames(); o++) {
            attachments[o][i] = views[o];
        }

        clearValues[i] = (depthOutput.value()->clear_value());

        i++;
    }

    pPass->set_clear_values(clearValues);

    std::vector<Framebuffer> framebuffers(num_frames(), VK_NULL_HANDLE);

    for(uint32_t f = 0; f < num_frames(); f++) {
        framebuffers[f] = FramebufferBuilder(rp,
                                             extent_for(pPass->renderpass()),
                                             attachments[f])
                                .build(pGpu);
    }

    return framebuffers;
}


int RenderGraphBuilder::build_renderpass(Gpu *pGpu, RenderGraphBuilderCache *pCache,
                                          RenderGraphVkCommandBuffer *pCmdBuf, RenderGraphNode *pPass,
                                          int depth, uint32_t renderpassIdx) {
    if(pPass->is_visited()) return 0;

    for(uint32_t i = 0; i < m_numRenderpasses; i++) {
        if(pCache->adjacency_matrix().get(i, renderpassIdx)) {
            RenderGraphVkCommandBuffer *pNextCmdBuf = pCmdBuf;
            if(pCache->adjacency_matrix().num_dependencies(renderpassIdx) > 1) {
                pNextCmdBuf = new RenderGraphVkCommandBuffer(pGpu, num_frames());
                pCmdBuf->add_dependency(pNextCmdBuf);
            }

            depth = build_renderpass(pGpu, pCache, pNextCmdBuf, &m_renderpasses[i], depth, i);
        }
    }

    /* does not matter, how many framebuffers returns */
    auto rp = create_renderpass_for(pGpu, pPass);
    auto framebuffers = collect_attachments(pGpu, rp, pCache, pPass);
    auto extent = extent_for(pPass->renderpass());

    pCmdBuf->add_render_pass({
                                 pPass->renderpass(),
                                 rp,
                                 extent,
                                 pPass->clear_values(),
                                 framebuffers,
                                 pPass->renderpass()->outputs()[0]->name() == "swapchain"
                            });

    // wrap it into single structure
    auto outputInfo = RenderPassOutputInfo(rp, framebuffers, extent_for(pPass->renderpass()));
    auto buildInfo = RenderPassBuildInfo(pGpu, pCache, outputInfo, pCache->sampler(), num_frames());

    pPass->renderpass()->prepare(buildInfo);

    pPass->mark_visited();

    return 1;
}

std::vector<RenderGraphVkCommandBuffer>
RenderGraphBuilder::build_swapchain_renderpass(Gpu *pGpu, RenderGraphBuilderCache *pCache) {
    std::vector<uint32_t> dependencies;
    for(uint32_t i = 0; i < m_numRenderpasses; i++) {
        if(pCache->adjacency_matrix().get(i, m_numRenderpasses)) {
            dependencies.push_back(i);
        }
    }

    /* create command buffer for each dependency */
    std::vector<RenderGraphVkCommandBuffer> cmdbufs(dependencies.size(),
                                                    RenderGraphVkCommandBuffer(pGpu, num_frames()));

    uint32_t i = 0;
    for(auto dependency : dependencies) {
        int depth = build_renderpass(pGpu, pCache, &cmdbufs[i],
                                     &m_renderpasses[dependency], 0, dependency);

        if(depth == 0) {
            i++;
        }
    }

    return cmdbufs;
}

RenderGraph RenderGraphBuilder::build(Gpu *pGpu, const std::string& outputName, const ImageChain& outputChain) {
    printf("Building render graph\n");

    // Build matrix
    auto adjacencyMatrix = build_adjacency_matrix();
    adjacencyMatrix.transitive_reduction();

    auto sampler = create_attachment_sampler(pGpu);

    // Prepare cache
    RenderGraphBuilderCache cache(outputName, m_numFrames, adjacencyMatrix, sampler, outputChain);

    auto queue = build_swapchain_renderpass(pGpu, &cache);

    cache.transfer_layout(pGpu);

    return { pGpu, outputChain, queue, m_numFrames };
}

std::map<std::string, RenderPass*>* RenderGraphBuilder::build_outputs_table() {
    auto outputTable = new std::map<std::string, RenderPass*>();

    for(auto& renderpass : m_renderpasses) {
        for(auto& output : renderpass.renderpass()->outputs()) {
            outputTable->insert({output->name(), renderpass.renderpass()});
        }
    }

    return outputTable;
}

AdjacencyMatrix
RenderGraphBuilder::build_adjacency_matrix() {
    AdjacencyMatrix matrix(m_numRenderpasses + 1);

    for(uint32_t x = 0; x < m_numRenderpasses; x++) {
        for(uint32_t y = 0; y < m_numRenderpasses; y++) {
            if(x == y) continue;

            if(m_renderpasses[x].renderpass()->has_pass_dependency(m_renderpasses[y].renderpass()->name())) {
                matrix.set(y, x);
                continue;
            }

            for(auto& output : m_renderpasses[x].renderpass()->outputs()) {
                if(std::count(m_renderpasses[y].renderpass()->inputs().begin(), m_renderpasses[y].renderpass()->inputs().end(), output->name()) > 0) {
                    matrix.set(x, y);
                    break;
                }
            }
        }

        if(m_renderpasses[x].renderpass()->output("swapchain").has_value()) {
            matrix.set(x, m_numRenderpasses);
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
    };

    VkSampler sampler = VK_NULL_HANDLE;
    if(vkCreateSampler(pGpu->dev(), &samplerInfo, nullptr, &sampler)) {
        throw std::runtime_error("failed to create sampler");
    }

    return sampler;
}