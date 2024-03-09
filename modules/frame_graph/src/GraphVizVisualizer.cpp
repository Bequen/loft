#include <iostream>
#include "GraphVizVisualizer.h"

GraphVizVisualizer::GraphVizVisualizer(RenderGraphBuilder *pBuilder, RenderGraph *pGraph) :
m_pBuilder(pBuilder), m_pGraph(pGraph) {

}

void GraphVizVisualizer::visualize_command_buffer(FILE* pFile, RenderGraphVkCommandBuffer cmdbuf) {
    static int commandBufferId = 0;

    for(auto& dependency : cmdbuf.dependencies) {
        visualize_command_buffer(pFile, *dependency);
    }

    fprintf(pFile, "subgraph commandBuffer_%i {\n", commandBufferId++);

    for(uint32_t i = 0; i < cmdbuf.renderpasses.size(); i++) {
        if(i != 0) {
            fwrite(" -> ", 4, 1, pFile);
        }
        fprintf(pFile, "\"%p\"[label=<<table cellborder=\"0\"><tr><td colspan=\"2\">%s</td></tr><tr>", cmdbuf.renderpasses[i].pRenderPass, cmdbuf.renderpasses[i].pRenderPass->name().c_str());

        auto renderpass = cmdbuf.renderpasses[i].pRenderPass;

        fwrite("<td>", 4, 1, pFile);

        if(renderpass->num_inputs() != 0) {
            fwrite("<table cellspacing=\"0\" cellborder=\"0\">", sizeof("<table cellspacing=\"0\" cellborder=\"0\">"), 1, pFile);

            for (uint32_t x = 0; x < renderpass->num_inputs(); x++) {
                fprintf(pFile, "<tr><td port=\"i%i\">", x);
                fwrite(renderpass->inputs()[x].c_str(), renderpass->inputs()[x].length(), 1, pFile);
                fwrite("</td></tr>", sizeof("</td></tr>") - 1, 1, pFile);
            }

            fwrite("</table>", sizeof("</table>") - 1, 1, pFile);
        }

        fwrite("</td>", 5, 1, pFile);

        fwrite("<td>", 4, 1, pFile);

        if(renderpass->num_outputs() != 0) {
            fwrite("<table cellspacing=\"0\" cellborder=\"0\">", sizeof("<table cellspacing=\"0\" cellborder=\"0\">"), 1, pFile);

            for (uint32_t x = 0; x < renderpass->num_outputs(); x++) {
                fprintf(pFile, "<tr><td port=\"o%i\">", x);
                fwrite(renderpass->outputs()[x]->name().c_str(), renderpass->outputs()[x]->name().length(), 1, pFile);
                fwrite("</td></tr>", sizeof("</td></tr>") - 1, 1, pFile);
            }

            fwrite("</table>", sizeof("</table>") - 1, 1, pFile);
        }

        fwrite("</td>", 5, 1, pFile);

        fwrite("</tr></table>>]", sizeof("</tr></table>>]") - 1, 1, pFile);
    }

    fwrite("\n", 1, 1, pFile);

    fprintf(pFile, "}\n");
}

GraphVizVisualizer& GraphVizVisualizer::visualize_into(FILE* pFile) {
    fprintf(pFile, "digraph G {\n");

    for(auto& cmdbuf : m_pGraph->dependencies()) {
        visualize_command_buffer(pFile, cmdbuf);
    }

    auto outputsTable = m_pBuilder->build_outputs_table();

    for(auto& renderpass : m_pBuilder->renderpasses()) {
        for(uint32_t x = 0; x < renderpass.renderpass()->num_inputs(); x++) {
            auto dependency = outputsTable->find(renderpass.renderpass()->inputs()[x])->second;

            uint32_t y = 0;
            for(y = 0; y < dependency->num_outputs(); y++) {
                if(dependency->outputs()[y]->name() == renderpass.renderpass()->inputs()[x]) {
                    break;
                }
            }

            fprintf(pFile, "\"%p\":<o%i> -> \"%p\":<i%i> [constraint=false, style=dotted]\n", dependency, y, renderpass.renderpass(), x);
        }
    }

    auto adjacencyMatrix = m_pBuilder->build_adjacency_matrix(outputsTable);
    adjacencyMatrix.transitive_reduction();


    for(uint32_t x = 0; x < m_pBuilder->renderpasses().size(); x++) {
        for(uint32_t y = 0; y < m_pBuilder->renderpasses().size(); y++) {
            if(adjacencyMatrix.get(x, y)) {
                fprintf(pFile, "\"%p\" -> \"%p\"\n",
                        m_pBuilder->renderpasses()[x].renderpass(),
                        m_pBuilder->renderpasses()[y].renderpass());
            }
        }
    }

    fprintf(pFile, "}");

    return *this;
}