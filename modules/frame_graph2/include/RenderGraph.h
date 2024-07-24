//
// Created by martin on 7/21/24.
//

#ifndef LOFT_RENDERGRAPH_H
#define LOFT_RENDERGRAPH_H

namespace lft::fg {

class RenderGraph {
public:
    RenderGraph(std::string name, std::string output_name);

    RenderGraph& add_color_attachment(std::string name, VkFormat format);

    RenderGraph& set_depth_attachment(std::string name, VkFormat format);

    RenderGraph& add_input(std::string name);
};

}

#endif //LOFT_RENDERGRAPH_H
