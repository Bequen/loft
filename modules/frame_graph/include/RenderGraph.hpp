#pragma once

#include <optional>
#include <vector>
#include <map>
#include <string>
#include <vulkan/vulkan_core.h>
#include <stdexcept>

#include "ResourceLayout.hpp"
#include "RenderPass.hpp"
#include "Resource.h"
#include "Swapchain.hpp"
#include "RenderGraphBuilderCache.h"
#include "RenderGraphNode.h"

struct Frame {
	VkFence readyFence;
	VkFence imageReadyFence;
};

class RenderGraph {
private:
	Gpu *m_pGpu;
	
	RenderGraphNode* m_root;
	Swapchain *m_pSwapchain;

	/* Information about frame index */
	uint32_t m_frameIdx;

	/* Exact number of images in flight that this RenderGraph was built from */
	const uint32_t m_numFrames;

	/* Each frame in flight is just a sequence of computation. */
	std::vector<Frame> m_frames;

	void prepare_frames();

    void run_swapchain_node(RenderGraphNode *pNode,
                            const uint32_t imageIdx, const uint32_t frameIdx,
                            VkFence fence) const;

	void run_tree(const RenderGraphNode *pNode, const uint32_t imageIdx) const;

    void print_dot_node(RenderGraphNode *pNode, RenderGraphNode *pParent, FILE *pOut) {
        if(pParent != nullptr) {
            fprintf(pOut, "\"%p\" -> \"%p\"\n", pNode, pParent);
        }

        for(auto& dependency : pNode->dependencies()) {
            print_dot_node(dependency, pNode, pOut);
        }
    }

public:
	RenderGraph(Gpu *pGpu, Swapchain *pSwapchain, RenderGraphNode* root, uint32_t numFrames);
	
	void run();

	void print() {
		print(m_root);
	}

	void print(const RenderGraphNode *pPass) {
		printf("%s\n", pPass->renderpass()->name().c_str());

        if(!pPass->dependencies().empty()) {
            printf("{\n");
            for (auto& dep : pPass->dependencies()) {
                print(dep);
            }
            printf("}\n");
        }
	}

    void print_dot(FILE *pOut) {
        fprintf(pOut, "digraph G {\n"
                      "graph [\n"
                      "rankdir = \"LR\"\n"
                      "];\n");

        print_dot_node(m_root, nullptr, pOut);
        fprintf(pOut, "}\n");
    }

    void
    record_node(const RenderGraphNode *pNode, VkCommandBuffer cmdbuf, Framebuffer framebuffer,
                uint32_t imageIdx) const;

    std::vector<VkSemaphore>
    run_dependencies(const RenderGraphNode *pNode, const uint32_t imageIdx) const;
};



