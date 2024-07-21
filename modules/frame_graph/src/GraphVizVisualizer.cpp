#include <iostream>
#include "GraphVizVisualizer.h"

GraphVizVisualizer::GraphVizVisualizer() {

}

void GraphVizVisualizer::visualize_render_pass(FILE *pFile, const GraphVizRenderPass &renderpass) {
    fprintf(pFile, "\"%s\"[label=\"%s\"]\n",
            renderpass.m_renderpass.pRenderPass->name().c_str(),
            renderpass.m_renderpass.pRenderPass->name().c_str());

    for(auto& output : renderpass.m_renderpass.pRenderPass->outputs()) {
        fprintf(pFile, "\"%s\"[shape=Mrecord]\n", output->name().c_str());
        fprintf(pFile, "\"%s\" -> \"%s\"\n",
                renderpass.m_renderpass.pRenderPass->name().c_str(),
                output->name().c_str());
    }

    if(renderpass.m_renderpass.pRenderPass->depth_output().has_value()) {
        auto depthOutput = renderpass.m_renderpass.pRenderPass->depth_output().value();
        fprintf(pFile, "\"%s\"[shape=Mrecord]\n", depthOutput->name().c_str());
        fprintf(pFile, "\"%s\" -> \"%s\"\n",
                renderpass.m_renderpass.pRenderPass->name().c_str(),
                depthOutput->name().c_str());
    }

    for(auto& input : renderpass.m_renderpass.pRenderPass->inputs()) {
        fprintf(pFile, "\"%s\"[shape=Mrecord]\n", input.c_str());
        fprintf(pFile, "\"%s\" -> \"%s\"\n",
                input.c_str(),
                renderpass.m_renderpass.pRenderPass->name().c_str());
    }
}

void GraphVizVisualizer::visualize_command_buffer(FILE* pFile, const GraphVizCommandBuffer& cmdbuf) {
    fprintf(pFile, "subgraph {\n");

    for(auto& renderpass : cmdbuf.m_renderpasses) {
        visualize_render_pass(pFile, renderpass);
    }

    for(auto& dependency : cmdbuf.m_dependencies) {
        visualize_command_buffer(pFile, dependency);
    }

    fprintf(pFile, "}\n");
}

void GraphVizVisualizer::visualize_render_graph(FILE* pFile, const GraphVizRenderGraph& graph) {
    fprintf(pFile, "subgraph %s {\n", graph.m_graph->name().c_str());

    /* for(auto& cmdbuf : graph.m_cmdbufs) {
        visualize_command_buffer(pFile, cmdbuf);
    } */

    auto matrix = graph.builder()->build_adjacency_matrix();
    matrix.transitive_reduction();

    for(uint32_t x = 0; x < graph.builder()->renderpasses().size(); x++) {
        for(uint32_t y = 0; y < graph.builder()->renderpasses().size(); y++) {
            if(matrix.get(x, y)) {
                fprintf(pFile, "\"%s\" -> \"%s\"\n",
                    graph.builder()->renderpasses()[x].renderpass()->name().c_str(),
                    graph.builder()->renderpasses()[y].renderpass()->name().c_str());
            }
        }
    }

    fprintf(pFile, "}\n");
}

GraphVizVisualizer& GraphVizVisualizer::visualize_into(FILE* pFile) {
    fprintf(pFile, "digraph G {\n");
    fprintf(pFile, "node[shape=rect]; rankdir=LR;\n");

    // divide nodes to subgraphs.
    // print_subgraphs();

    fprintf(pFile, "}\n");

    return *this;
}

GraphVizVisualizer &GraphVizVisualizer::add_graph(const std::shared_ptr<const RenderGraphBuilder> builder,
                                                  const std::shared_ptr<const RenderGraph> graph) {
    m_graphs.push_back(GraphVizRenderGraph(builder, graph));

    return *this;
}


