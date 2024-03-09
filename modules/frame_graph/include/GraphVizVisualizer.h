//
// Created by martin on 3/8/24.
//

#ifndef LOFT_GRAPHVIZVISUALIZER_H
#define LOFT_GRAPHVIZVISUALIZER_H

#include "RenderGraphBuilder.h"

class GraphVizVisualizer {
private:
    RenderGraphBuilder *m_pBuilder;
    RenderGraph *m_pGraph;

    void visualize_command_buffer(FILE* pFile, RenderGraphVkCommandBuffer cmdbuf);

public:
    GraphVizVisualizer(RenderGraphBuilder *pBuilder, RenderGraph *pGraph);

    GraphVizVisualizer& visualize_into(FILE *pFile);
};

#endif //LOFT_GRAPHVIZVISUALIZER_H
