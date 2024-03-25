//
// Created by martin on 3/8/24.
//

#ifndef LOFT_GRAPHVIZVISUALIZER_H
#define LOFT_GRAPHVIZVISUALIZER_H

#include <utility>

#include "RenderGraphBuilder.h"

struct GraphVizRenderPassNode {
    RenderPass *m_pRenderPass;

    GraphVizRenderPassNode(RenderPass *pPass) :
        m_pRenderPass(pPass) {

    }
};

struct GraphVizRenderPass {
    RenderGraphVkRenderPass m_renderpass;

    explicit GraphVizRenderPass(RenderGraphVkRenderPass renderpass) :
            m_renderpass(std::move(renderpass)) {

    }
};

struct GraphVizCommandBuffer {
    RenderGraphVkCommandBuffer m_cmdbuf;

    std::vector<GraphVizRenderPass> m_renderpasses;
    std::vector<GraphVizCommandBuffer> m_dependencies;

    explicit GraphVizCommandBuffer(RenderGraphVkCommandBuffer cmdbuf) :
            m_cmdbuf(std::move(cmdbuf)) {

        for(auto& renderpass : m_cmdbuf.renderpasses) {
            m_renderpasses.push_back(GraphVizRenderPass(renderpass));
        }

        for(auto& dependency : m_cmdbuf.dependencies) {
            m_dependencies.push_back(GraphVizCommandBuffer(*dependency));
        }
    }
};

struct GraphVizRenderGraph {
    const RenderGraph* m_pGraph;
    const RenderGraphBuilder *m_pBuilder;

    std::vector<GraphVizCommandBuffer> m_cmdbufs;

    GET(m_pGraph, graph);
    GET(m_pBuilder, builder);
    GET(m_cmdbufs, command_buffers);

    GraphVizRenderGraph(const RenderGraph *pGraph, const RenderGraphBuilder *pBuilder) :
        m_pGraph(pGraph), m_pBuilder(pBuilder) {

        for(auto& dep : pGraph->dependencies()) {
            m_cmdbufs.push_back(GraphVizCommandBuffer(dep));
        }
    }
};

class GraphVizVisualizer {
private:
    std::vector<GraphVizRenderGraph> m_graphs;

    void visualize_render_pass(FILE *pFile, const GraphVizRenderPass& renderpass);

    void visualize_command_buffer(FILE* pFile, const GraphVizCommandBuffer& cmdbuf);

    void visualize_render_graph(FILE *pFile, const GraphVizRenderGraph& renderGraph);

public:
    GraphVizVisualizer();

    GraphVizVisualizer& visualize_into(FILE *pFile);

    GraphVizVisualizer& add_graph(const RenderGraphBuilder *pBuilder, const RenderGraph *pGraph);
};

#endif //LOFT_GRAPHVIZVISUALIZER_H
