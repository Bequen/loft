#pragma once

#include <utility>
#include <vector>
#include <map>
#include <string>
#include <cassert>
#include <memory>

#include "Resource.h"
#include "Swapchain.hpp"
#include "AdjacencyMatrix.h"
#include "ImageChain.h"

struct RenderGraphFrame {
    std::vector<std::unique_ptr<Resource>> m_resources;

    RenderGraphFrame& cache_image(uint32_t idx, Image image, ImageView view, bool isDepth) {
        if (m_resources.size() <= idx) {
            m_resources.resize(idx + 1);
        }

        m_resources[idx] = std::make_unique<ImageResource>(ImageResource(image, view, isDepth));

        return *this;
    }

    RenderGraphFrame& cache_buffer(uint32_t idx, Buffer buffer) {
        m_resources[idx] = std::make_unique<BufferResource>(BufferResource(std::move(buffer)));

        return *this;
    }

    ImageResource* get_image_view(uint32_t idx) {
        return (ImageResource*)m_resources[idx].get();
    }
};

union VirtualResource {
public:
    Buffer m_buffer;
    Image m_image;
};

class RenderGraphResourceManager {
public:
    RenderGraphResourceManager(const RenderGraphResourceManager&) = delete;
};

/**
 * Saving intermediate computations for render graph builder
 */
class RenderGraphBuilderCache {
private:
    std::string m_outputName;

    std::map<std::string, uint32_t> m_resourceMap;
    std::vector<RenderGraphFrame> m_frame;
	uint32_t m_numCachedResources;

    AdjacencyMatrix m_adjacencyMatrix;

    VkSampler m_sampler;
    const ImageChain& m_outputChain;


public:
    GET(m_outputChain, output_chain);
    GET(m_sampler, sampler);
    GET(m_adjacencyMatrix, adjacency_matrix);
    GET(m_outputName, output_name);

    RenderGraphBuilderCache(std::string outputName, uint32_t numFrames,
                            AdjacencyMatrix adjacencyMatrix,
                            VkSampler sampler, const ImageChain& outputChain) :
    m_outputName(std::move(outputName)),
    m_frame(numFrames),
    m_adjacencyMatrix(std::move(adjacencyMatrix)),
    m_outputChain(outputChain),
    m_numCachedResources(0),
    m_sampler(sampler) {

    }

    uint32_t transfer_layout(const std::shared_ptr<const Gpu>& gpu) {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = gpu->graphics_command_pool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(gpu->dev(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        std::vector<std::vector<VkImageMemoryBarrier>> frames(m_frame.size());

        for(uint32_t f = 0; f < m_frame.size(); f++) {
            std::vector<VkImageMemoryBarrier> barriers(m_frame[f].m_resources.size(), barrier);
            for (int i = 0; i < m_frame[f].m_resources.size(); i++) {
                if(m_frame[f].m_resources[i]->as_image()->is_depth()) {
                    barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                }
                barriers[i].image = ((ImageResource*)m_frame[f].m_resources[i].get())->image.img;
            }

            vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    barriers.size(), barriers.data()
            );

            frames[f] = barriers;
        }

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        gpu->enqueue_transfer(&submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(gpu->transfer_queue());

        return 0;
    }

    RenderGraphBuilderCache&
    cache_image(const std::string& name,
				std::vector<Image>& images,
				std::vector<ImageView>& views,
                bool isDepth = false) {
        assert(images.size() == m_frame.size() && 
			   views.size() == m_frame.size());

		m_resourceMap[name] = m_numCachedResources;
		
        for(uint32_t i = 0; i < m_frame.size(); i++) {
            m_frame[i].cache_image(m_numCachedResources, images[i], views[i], isDepth);
        }

		m_numCachedResources++;

        return *this;
    }

    ImageResource* get_image(const std::string& name, uint32_t frameIdx) {
        assert(frameIdx < m_frame.size());
        assert(m_resourceMap.count(name) > 0);

        return m_frame[frameIdx].get_image_view(m_resourceMap[name]);
    }
};
