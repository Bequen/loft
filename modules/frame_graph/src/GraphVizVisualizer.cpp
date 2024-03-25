#include <iostream>
#include "GraphVizVisualizer.h"

GraphVizVisualizer::GraphVizVisualizer() {

}

void GraphVizVisualizer::visualize_render_pass(FILE *pFile, const GraphVizRenderPass &renderpass) {

}

void GraphVizVisualizer::visualize_command_buffer(FILE* pFile, const GraphVizCommandBuffer& cmdbuf) {
    fprintf(pFile, "subgraph {\n");

    for(auto& renderpass : cmdbuf.m_renderpasses) {
        visualize_render_pass(pFile, renderpass);
    }

    fprintf(pFile, "}\n");
}

void GraphVizVisualizer::visualize_render_graph(FILE* pFile, const GraphVizRenderGraph& graph) {
    fprintf(pFile, "subgraph %s {\n", graph.m_pGraph->name().c_str());

    for(auto& cmdbuf : graph.m_cmdbufs) {
        visualize_command_buffer(pFile, cmdbuf);
    }

    fprintf(pFile, "}\n");
}

GraphVizVisualizer& GraphVizVisualizer::visualize_into(FILE* pFile) {
    fprintf(pFile, "digraph G {\n");
    fprintf(pFile, "node[shape=rect]; rankdir=LR;\n");

    for(auto& graph : m_graphs) {
        visualize_render_graph(pFile, graph);
    }

    fprintf(pFile, "}\n");

    return *this;
}

GraphVizVisualizer &GraphVizVisualizer::add_graph(const RenderGraphBuilder *pBuilder, const RenderGraph *pGraph) {
    m_graphs.push_back(GraphVizRenderGraph(pGraph, pBuilder));

    return *this;
}


