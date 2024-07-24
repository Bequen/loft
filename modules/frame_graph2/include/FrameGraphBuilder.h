//
// Created by martin on 7/21/24.
//

#ifndef LOFT_RENDERGRAPHBUILDER_H
#define LOFT_RENDERGRAPHBUILDER_H

#include <memory>

#include "FrameGraph.h"
#include "RenderGraph.h"

namespace lft::fg {

class FrameGraphBuilder {
public:
    FrameGraphBuilder& add_render_graph(const std::shared_ptr<const RenderGraph> render_graph);

    FrameGraphBuilder& add_external_image();

    FrameGraphBuilder& add_external_buffer();

    FrameGraph build();
};

}

#endif //LOFT_RENDERGRAPHBUILDER_H
