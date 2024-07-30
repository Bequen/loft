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

        /* for(auto& renderpass : m_cmdbuf.m_renderpasses) {
            m_renderpasses.push_back(GraphVizRenderPass(renderpass));
        }

        for(auto& dependency : m_cmdbuf.dependencies) {
            m_dependencies.push_back(GraphVizCommandBuffer(*dependency));
        } */
    }
};

struct GraphVizRenderGraph {
    const std::shared_ptr<const RenderGraphBuilder> m_builder;
    const std::shared_ptr<const RenderGraph> m_graph;
    std::vector<GraphVizCommandBuffer> m_cmdbufs;

    REF(m_builder, builder);
    REF(m_cmdbufs, command_buffers);

    GraphVizRenderGraph(const std::shared_ptr<const RenderGraphBuilder> builder,
                        const std::shared_ptr<const RenderGraph> graph) :
         m_builder(builder),
         m_graph(graph) {

        for(auto& dep : graph->dependencies()) {
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

    GraphVizVisualizer& add_graph(const std::shared_ptr<const RenderGraphBuilder> builder,
                                  const std::shared_ptr<const RenderGraph> graph);
};

#endif //LOFT_GRAPHVIZVISUALIZER_H
